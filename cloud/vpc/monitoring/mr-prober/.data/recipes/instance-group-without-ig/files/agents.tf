resource "ycp_compute_image" "agent" {
  folder_id = var.folder_id

  name = "${var.prefix}-agent"
  description = "Image for agent built with packer from Mr.Prober images collection"
  uri = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/agent/${var.mr_prober_agent_image_name}.qcow2"

  os_type = "LINUX"
  min_disk_size = 10
  # We have no quota for disk pools yet
  # pooled = true

  depends_on = [
    ycp_resource_manager_folder.folder
  ]
}

resource "ycp_compute_instance" "agents" {
  for_each = {
    for index in range(var.vm_count) : index => {
      zone_id = var.zones[index % length(var.zones)],
      name = "${var.prefix}-agent-${var.zone_to_dc[var.zones[index % length(var.zones)]]}${floor(index / length(var.zones)) + 1}",
      fqdn = "${var.zone_to_dc[var.zones[index % length(var.zones)]]}${floor(index / length(var.zones)) + 1}.${var.prefix}.${var.dns_zone}"
    }
  }

  folder_id = var.folder_id

  name        = each.value.name
  hostname    = each.value.fqdn
  fqdn        = each.value.fqdn

  description = "Just one of agents for ${var.prefix} cluster"

  service_account_id = var.mr_prober_sa_id

  zone_id = each.value.zone_id

  boot_disk {
    disk_spec {
      description = "Primary boot disk with operating system, agent container and logs"
      image_id    = ycp_compute_image.agent.id
      size        = var.vm_boot_disk_size
    }
  }

  dynamic "secondary_disk" {
    for_each = var.add_secondary_disks ? ["disk"] : []
    content {
      disk_spec {
        description = "Disk for monitoring tests only"
        type_id     = "network-ssd"
        size        = 50
      }
    }
  }

  resources {
    memory        = var.vm_memory
    cores         = var.vm_cores
    core_fraction = var.vm_core_fraction
  }

  # Just a placeholder. See https://st.yandex-team.ru/CLOUD-97056
  gpu_settings {}

  dynamic "network_interface" {
    for_each = var.create_monitoring_network ? ["monitoring"]: []
    content {
      subnet_id          = lookup(lookup(ycp_vpc_subnet.subnets, each.value.zone_id), "id")
      security_group_ids = [ycp_vpc_security_group.agents[0].id]
      primary_v4_address {
        dynamic "one_to_one_nat" {
          for_each = var.monitoring_network_add_floating_ip ? ["fip"] : []
          content {
            ip_version = "IPV4"
          }
        }
      }
      primary_v6_address {
      }
    }
  }

  network_interface {
    subnet_id          = lookup(var.control_network_subnet_ids, each.value.zone_id)
    primary_v4_address {
    }
    primary_v6_address {
      dns_record {
        dns_zone_id = var.dns_zone_id
        fqdn        = "${each.value.fqdn}."
        ptr         = true
      }
    }
  }

  metadata = {
    user-data = templatefile(
      "${path.module}/cloud-init.yaml",
      {
        hostname                       = each.value.fqdn,
        stand_name                     = local.mr_prober_environment,
        cluster_id                     = var.cluster_id,
        agent_additional_metric_labels = var.agent_additional_metric_labels,
        s3_endpoint                    = var.s3_endpoint,
        runcmd                         = var.cloud_init_runcmd,
        bootcmd                        = var.cloud_init_bootcmd
      }
    )
    enable-oslogin = "true"
    skm            = local.skm_metadata

    # See https://wiki.yandex-team.ru/cloud/devel/instance-group/internal/
    # Skm keys are regenerated on Creator restart, so ignore them.
    # If you want update secrets for agents, add some new metadata key (i.e. "secrets_versions = 1.0")
    internal-metadata-live-update-keys = "internal-metadata-live-update-keys,skm"
  }

  labels = {
    abc_svc = "ycvpc"
    layer   = "iaas"
    env     = var.label_environment
  }

  platform_id = var.vm_platform_id
}

resource "ytr_conductor_host" "agents" {
  count = var.use_conductor ? var.vm_count : 0

  fqdn = ycp_compute_instance.agents[count.index].fqdn
  short_name = ycp_compute_instance.agents[count.index].fqdn
  datacenter_id = lookup(local.ytr_conductor_datacenter_by_zone, ycp_compute_instance.agents[count.index].zone_id)
  group_id = ytr_conductor_group.agents_in_datacenter[var.zone_to_dc[ycp_compute_instance.agents[count.index].zone_id]].id
}

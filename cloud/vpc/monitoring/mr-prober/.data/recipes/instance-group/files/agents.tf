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

resource "ycp_microcosm_instance_group_instance_group" "agents" {
  folder_id = var.folder_id

  name        = "${var.prefix}-agents"
  description = "Group of agents for running tests"

  service_account_id = var.mr_prober_sa_id

  instance_template {
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

    dynamic "network_interface" {
      for_each = var.create_monitoring_network ? ["monitoring"]: []
      content {
        network_id         = ycp_vpc_network.network[0].id
        subnet_ids         = [for subnet in ycp_vpc_subnet.subnets : subnet.id]
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
      network_id = var.control_network_id
      subnet_ids = values(var.control_network_subnet_ids)
      primary_v4_address {
      }
      primary_v6_address {
        dns_record_spec {
          dns_zone_id = var.dns_zone_id
          fqdn        = "{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}."
          ptr         = true
        }
      }
    }

    name        = "${var.prefix}-{instance.internal_dc}{instance.index_in_zone}"
    description = "Just one of agents for ${var.prefix} cluster"
    hostname    = "{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}"
    fqdn        = "{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}"

    metadata = {
      user-data = templatefile(
        "${path.module}/cloud-init.yaml",
        {
          hostname                       = "{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}",
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

    service_account_id = var.mr_prober_sa_id
  }

  allocation_policy {
    dynamic zone {
      for_each = var.zones
      content {
        zone_id = zone.value
      }
    }
  }

  deploy_policy {
    max_unavailable = 2
    max_creating    = 10
    max_expansion   = 2
    max_deleting    = 2
  }

  scale_policy {
    fixed_scale {
      size = var.vm_count
    }
  }
}

data "yandex_compute_instance_group" "agents" {
   instance_group_id = ycp_microcosm_instance_group_instance_group.agents.id
}

locals {
  agent_instances = data.yandex_compute_instance_group.agents.instances
}

resource "ytr_conductor_host" "agents" {
  count = var.use_conductor ? ycp_microcosm_instance_group_instance_group.agents.scale_policy[0].fixed_scale[0].size : 0

  fqdn = local.agent_instances[count.index].fqdn
  short_name = local.agent_instances[count.index].fqdn
  datacenter_id = lookup(local.ytr_conductor_datacenter_by_zone, local.agent_instances[count.index].zone_id)
  group_id = ytr_conductor_group.agents_in_datacenter[local.agent_instances[count.index].zone_id].id
}

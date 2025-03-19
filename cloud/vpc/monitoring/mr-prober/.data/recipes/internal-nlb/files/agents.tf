resource "ycp_compute_image" "agent" {
  folder_id = var.folder_id

  name = "${var.prefix}-agent"
  description = "Image for agent built with packer from Mr.Prober images collection"
  uri = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/agent/fd8a5nv18vv5u40duiig.qcow2"

  os_type = "LINUX"
  min_disk_size = 10
}

resource "ycp_microcosm_instance_group_instance_group" "agents" {
  folder_id = var.folder_id

  name = "${var.prefix}-agents"
  description = "Group of agents for running tests"

  service_account_id = var.mr_prober_sa_id

  instance_template {
    boot_disk {
      disk_spec {
        image_id = ycp_compute_image.agent.id
        size = 10
      }
    }

    resources {
      memory = 2
      cores = 2
      core_fraction = 20
    }

    network_interface {
      network_id = ycp_vpc_network.network.id
      subnet_ids = [for s in ycp_vpc_subnet.subnets : s.id]
      primary_v4_address {}
      primary_v6_address {}
    }

    network_interface {
      network_id = var.control_network_id
      subnet_ids = values(var.control_network_subnet_ids)
      primary_v6_address {
        dns_record_spec {
          fqdn = "agent-{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}."
          dns_zone_id = var.dns_zone_id
          ptr = true
        }
      }
    }

    name = "${var.prefix}-agent-{instance.internal_dc}{instance.index_in_zone}"
    description = "Just one of agents for simple-internal-load-balancer cluster"
    hostname = "agent-{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}"
    fqdn = "agent-{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}"

    metadata = {
      user-data = templatefile(
        "${path.module}/cloud-init.yaml",
        {
          hostname = "agent-{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}",
          stand_name = local.mr_prober_environment,
          cluster_id = var.cluster_id,
          agent_additional_metric_labels = var.agent_additional_metric_labels,
          s3_endpoint = var.s3_endpoint,
          runcmd = [],
          bootcmd = []
        }
      )
      enable-oslogin = "true"
      skm = local.skm_metadata

      # See https://wiki.yandex-team.ru/cloud/devel/instance-group/internal/
      # Skm keys are regenerated on Creator restart, so ignore them.
      # If you want update secrets for agents, add some new metadata key (i.e. "secrets_versions = 1.0")
      internal-metadata-live-update-keys = "internal-metadata-live-update-keys,skm"
    }

    labels = {
      abc_svc = "ycvpc"
      layer = "iaas"
      env = var.label_environment
    }

    platform_id = "standard-v2"

    service_account_id = var.mr_prober_sa_id
  }

  allocation_policy {
    dynamic zone {
      for_each = var.monitoring_network_zones
      content {
        zone_id = zone.value
      }
    }
  }

  deploy_policy {
    max_unavailable = 1
    max_creating = 3
    max_expansion = 1
    max_deleting = 1
  }

  scale_policy {
    fixed_scale {
      size = length(var.monitoring_network_zones)
    }
  }
}
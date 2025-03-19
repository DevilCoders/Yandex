resource "ycp_compute_image" "web_server" {
  folder_id = var.folder_id

  name = "${var.prefix}-web-server"
  description = "Image for web server built with packer from Mr.Prober images collection"
  uri = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/web-server/fd80futih1imolei7857.qcow2"

  os_type = "LINUX"
  min_disk_size = 10
}

resource "ycp_microcosm_instance_group_instance_group" "backends" {
  folder_id = var.folder_id

  name = "${var.prefix}-backends"
  description = "Group of backends behind of internal network load balancer"

  service_account_id = var.mr_prober_sa_id

  instance_template {
    boot_disk {
      disk_spec {
        image_id = ycp_compute_image.web_server.id
        size = 10
      }
    }

    resources {
      memory = 1
      cores = 2
      core_fraction = 5
    }

    network_interface {
      network_id = ycp_vpc_network.network.id
      subnet_ids = [for s in ycp_vpc_subnet.subnets : s.id]
      security_group_ids = [ycp_vpc_security_group.backends.id]
      primary_v4_address {
        name = "v4"
      }
      primary_v6_address {
        name = "v6"
      }
    }

    network_interface {
      network_id = var.control_network_id
      subnet_ids = values(var.control_network_subnet_ids)
      primary_v6_address {
        dns_record_spec {
          fqdn = "backend-{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}."
          dns_zone_id = var.dns_zone_id
          ptr = true
        }
      }
    }

    name = "${var.prefix}-backend-{instance.internal_dc}{instance.index_in_zone}"
    description = "Just one of backends for simple-internal-load-balancer cluster"
    hostname = "backend-{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}"

    metadata = {
      user-data = templatefile(
        "${path.module}/cloud-init.yaml",
        {
          hostname = "backend-{instance.internal_dc}{instance.index_in_zone}.${var.prefix}.${var.dns_zone}",
          stand_name = local.mr_prober_environment,
          cluster_id = var.cluster_id,
          agent_additional_metric_labels = var.agent_additional_metric_labels,
          s3_endpoint = var.s3_endpoint,
          runcmd = [],
          # Add route via eth0 (monitoring interface) to inter-network communication and healthcheck service
          bootcmd = [for cidr in concat(values(var.monitoring_network_ipv6_cidrs), var.healthcheck_service_source_ipv6_networks): format("ip -6 route add %s dev eth0", cidr)]
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
      for_each = var.vm_zones
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
      size = 2 * length(var.vm_zones)
    }
  }

  health_checks_spec {
    health_check_spec {
      address_names = ["v4"]
      interval = "20s"
      timeout = "5s"
      healthy_threshold = 2
      unhealthy_threshold = 3
      http_options {
        port = 80
        path = "/"
      }
    }
  }
}

data "yandex_compute_instance_group" "backends" {
   instance_group_id = ycp_microcosm_instance_group_instance_group.backends.id
}

locals {
  backend_instances = data.yandex_compute_instance_group.backends.instances
}
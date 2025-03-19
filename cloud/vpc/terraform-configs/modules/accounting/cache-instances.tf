resource "ycp_microcosm_instance_group_instance_group" "vpc_cache" {
  folder_id = var.folder_id

  name = "cache-ig"
  description = "Group of yc-vpc-accounting-cache services"
  service_account_id = ycp_iam_service_account.yc_vpc_accounting_ig_sa.id
  labels = {}

  instance_template {
    name = "vpc-cache-{instance.index_in_zone}-{instance.internal_dc}"

    description = "Just one of cache accounting backends"
    hostname = "{instance.index_in_zone}.{instance.internal_dc}.cache.${var.dns_zone}"
    service_account_id = ycp_iam_service_account.yc_vpc_billing_sa.id
    
    boot_disk {
      disk_spec {
        image_id = var.accounting_image
        size = 15
      }
    }

    platform_id = var.vm_platform_id
    resources {
      memory = 16
      cores = 16
    }

    network_interface {
      network_id = ycp_vpc_network.network.id
      subnet_ids = [for s in ycp_vpc_subnet.subnets : s.id]
      security_group_ids = [yandex_vpc_security_group.accounting.id]
      
      primary_v4_address {
        name = "ipv4"
      }
      primary_v6_address {
        name = "ipv6"
        dns_record_spec {
          fqdn = "{instance.index_in_zone}.{instance.internal_dc}.cache.${var.dns_zone}."
          dns_zone_id = var.dns_zone_id
          ptr = true
        }
        dns_record_spec {
          fqdn = "cache.${var.dns_zone}." 
          dns_zone_id = var.dns_zone_id
        }
      }
    }

    metadata = {
      user-data = templatefile("${path.module}/files/cloud-init.yaml",
        {
          hostname = "{instance.index_in_zone}.{instance.internal_dc}.cache.${var.dns_zone}",
          files = [
              {
                path = "/etc/yc/vpc-accounting/accounting.env"
                content = templatefile("${path.module}/files/vpc-accounting/accounting.env", {
                  accounting_cmd = "yc-vpc-accounting-cache"
                })
              },
              {
                path = "/etc/yc/vpc-accounting-cache/config.yaml",
                content = templatefile("${path.module}/files/vpc-accounting-cache/config.yaml", {
                  log_level = var.log_level
                  environment = var.ycp_profile
                  pull_interval = var.solomon_pull_interval
                })
              },
              {
                path = "/etc/.stand",
                content = var.ycp_profile
              }
          ]
        }
      )
      enable-oslogin = "true"
    }

    labels = {
      layer = "iaas"
      abc_svc = "ycvpc"
      env = var.environment
    }
  }

  allocation_policy {
    dynamic zone {
      for_each = var.accounting_network_subnet_zones
      content {
        zone_id = zone.value
      }
    }
  }

  deploy_policy {
    max_unavailable = 0
    max_creating = 1
    max_expansion = 1
    max_deleting = 1
  }

  scale_policy {
    fixed_scale {
      size = 1
    }
  }

  health_checks_spec {
    health_check_spec {
      address_names = ["ipv6"]
      interval = "20s"
      timeout = "5s"
      healthy_threshold = 2
      unhealthy_threshold = 9
      http_options {
        port = 80
        path = "/"
      }
    }
  }
}

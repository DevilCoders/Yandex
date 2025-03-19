resource "ycp_microcosm_instance_group_instance_group" "yc_search_proxy" {
  name               = "ycsearch-proxy"
  folder_id          = var.folder_id
  service_account_id = var.service_account_id

  allocation_policy {
    zone {
      zone_id = local.zones.zone_a.id
    }
    zone {
      zone_id = local.zones.zone_b.id
    }
    zone {
      zone_id = local.zones.zone_c.id
    }
  }

  deploy_policy {
    max_expansion   = 0
    max_unavailable = 1
  }

  instance_template {
    name               = join("", ["ycsearch-proxy", var.instance_name_suffix])
    fqdn               = join("", ["ycsearch-proxy", var.instance_name_suffix, var.domain])
    service_account_id = var.service_account_id
    platform_id        = var.instance_platform

    boot_disk {
      disk_spec {
        image_id = local.services.yc_search_proxy.image_id
        type_id  = "network-hdd"
        size     = 10
      }
    }

    resources {
      cores         = 2
      memory        = 8
      core_fraction = 100
    }

    metadata = {
      "serial-port-enable" = "1"
      "user-data"          = module.metadata["yc_search_proxy"].metadata
    }

    network_interface {
      subnet_ids = [
        ycp_vpc_subnet.yc-search-preprod-nets-ru-central1-a.id,
        ycp_vpc_subnet.yc-search-preprod-nets-ru-central1-b.id,
        ycp_vpc_subnet.yc-search-preprod-nets-ru-central1-c.id,
      ]
      primary_v4_address {}
      primary_v6_address {}
    }

    secondary_disk {
      disk_spec {
        size    = 10
        type_id = "network-ssd"
      }
    }

    secondary_disk {
      disk_spec {
        size    = 30
        type_id = "network-hdd"
      }
    }
  }

  scale_policy {
    fixed_scale {
      size = 3
    }
  }

  //  application_load_balancer_spec {
  //    target_group_spec {
  //      name = "ycsearch-proxy-prod-alb-group"
  //    }
  //  }

  // this is legacy support for `terraform plan` zero diff =(
  // it should be deleted later
  platform_l7_load_balancer_spec {
    preferred_ip_version = "IPV6"
    target_group_spec {
      name = "ycsearch-proxy-preprod-alb-group"
    }
  }

  health_checks_spec {
    health_check_spec {
      http_options {
        path = "/ping"
        port = 8080
      }
      interval            = "15s"
      timeout             = "10s"
      unhealthy_threshold = 3
      healthy_threshold   = 2
    }
  }

  variable {
    key   = "ru-central1-a_shortname"
    value = local.zones.zone_a.shortname
  }
  variable {
    key   = "ru-central1-b_shortname"
    value = local.zones.zone_b.shortname
  }
  variable {
    key   = "ru-central1-c_shortname"
    value = local.zones.zone_c.shortname
  }
}

// it is used in communal L7 decribed here https://a.yandex-team.ru/arc/trunk/arcadia/cloud/platform/selfhost/terraform/preprod/l7/cpl-router/virtual_hosts.tf
resource "ycp_platform_alb_backend_group" "yc_search_proxy" {
  name = "yc-search-proxy"
  http {
    backend {
      name = "http-ping"
      port = 8080

      healthchecks {
        healthy_threshold   = 2
        interval            = "5s"
        timeout             = "2s"
        unhealthy_threshold = 2
        http {
          path = "/ping"
        }
      }

      target_group {
        target_group_id = ycp_microcosm_instance_group_instance_group.yc_search_proxy.application_load_balancer_state[0].target_group_id
      }

    }
  }
}

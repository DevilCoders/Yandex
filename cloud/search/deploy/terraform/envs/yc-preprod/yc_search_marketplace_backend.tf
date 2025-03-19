resource "ycp_microcosm_instance_group_instance_group" "yc_search_marketplace_backend" {
  name               = "ycsearch-marketplace-backend"
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
    name               = join("", ["ycsearch-mrkt-backend", var.instance_name_suffix])
    fqdn               = join("", ["ycsearch-mrkt-backend", var.instance_name_suffix, var.domain])
    service_account_id = var.service_account_id
    platform_id        = var.instance_platform

    boot_disk {
      disk_spec {
        image_id = local.services.yc_search_marketplace_backend.image_id
        type_id  = "network-hdd"
        size     = 10
      }
    }

    resources {
      cores         = 2
      memory        = 4
      core_fraction = 100
    }

    metadata = {
      "serial-port-enable" = "1"
      "user-data"          = module.metadata["yc_search_marketplace_backend"].metadata
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
        size    = 20
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

  health_checks_spec {
    health_check_spec {
      http_options {
        path = "/ping"
        port = 8081
      }
      interval            = "60s"
      timeout             = "59s"
      unhealthy_threshold = 10
      healthy_threshold   = 0
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

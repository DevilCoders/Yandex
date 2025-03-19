resource "ycp_vpc_address" "schecker-l7-ipv6" {
  name      = "schecker-l7-ipv6"
  reserved  = true
  folder_id = var.yc_folder
  labels = module.common.instance_labels

  ipv6_address_spec {
    requirements {
      hints = ["yandex-only"]
    }
  }
  lifecycle {
    prevent_destroy = true
  }
}

resource "ycp_platform_alb_load_balancer" "alb" {
  name       = "schecker-alb"
  folder_id  = var.yc_folder
  network_id = var.network_id
  security_group_ids = var.security_group_ids
  labels = module.common.instance_labels

  allocation_policy {
    dynamic "location" {
      for_each = var.zones
      content {
        zone_id   = location.value
        subnet_id = var.subnets[location.value]
      }
    }
  }

  listener {
    name = "schecker-alb-listener"
    endpoint {
      address {
        external_ipv6_address {
          address = ycp_vpc_address.schecker-l7-ipv6.ipv6_address[0].address
        }
      }
      ports = [
      443]
    }
    tls {
      default_handler {
        certificate_ids = [ ycp_certificatemanager_certificate_request.schecker-api.id ]
        http_handler {
          http_router_id = ycp_platform_alb_http_router.http_router.id
        }
      }
    }
  }
}

resource "ycp_platform_alb_http_router" "http_router" {
  name      = "schecker-alb-http-router"
  folder_id = var.yc_folder
  labels = module.common.instance_labels
}

resource "ycp_platform_alb_virtual_host" "virtual_host" {
  name           = "schecker-alb-virtual-host"
  http_router_id = ycp_platform_alb_http_router.http_router.id

  route {
    name = "schecker-route"
    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = ycp_platform_alb_backend_group.backend_group.id
        timeout      = "60s"
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "backend_group" {
  name      = "schecker-alb-backend-group"
  folder_id = var.yc_folder
  labels = module.common.instance_labels

  http {
    backend {
      name = "http-backend"
      port = 5000

      healthchecks {
        interval            = "1s"
        timeout             = "1s"
        healthy_threshold   = 3
        unhealthy_threshold = 2
        healthcheck_port    = 5000

        http {
          path = "/ping"
        }
      }

      target_group {
        target_group_id = ycp_platform_alb_target_group.target_group.id
      }
    }
  }
}

resource "ycp_platform_alb_target_group" "target_group" {
  name      = "schecker-alb-target-group"
  folder_id = var.yc_folder
  labels = module.common.instance_labels

  dynamic "target" {
    for_each = var.ipv6_addresses
    content {
      ip_address = target.value
      subnet_id  = var.subnets[var.custom_yc_zones[target.key]]
      locality {
        zone_id = var.custom_yc_zones[target.key]
      }
    }
  }
}

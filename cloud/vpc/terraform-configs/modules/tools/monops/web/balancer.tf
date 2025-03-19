locals {
  region_id = "ru-central1"
}

resource "ycp_vpc_address" "monops_addr" {
  folder_id = var.folder_id
  name      = "monops-lb-addr"

  reserved = true

  ipv6_address_spec {
    address = var.monops_ipv6_address
    requirements {
      hints = ["yandex-only"]
    }
  }
}

resource "ycp_load_balancer_network_load_balancer" "monops_nlb" {
  folder_id = var.folder_id
  name      = "monops-web-lb"
  region_id = local.region_id
  type      = "EXTERNAL"

  attached_target_group {
    // Use default empty list or plan will return "monops_web_ig.load_balancer is empty list of object"
    target_group_id = element(concat(ycp_microcosm_instance_group_instance_group.monops_web_ig.load_balancer_state.*.target_group_id, [""]), 0)

    health_check {
      name                = "monops-web-ping"
      interval            = "5s"
      timeout             = "2s"
      healthy_threshold   = 4
      unhealthy_threshold = 2
      http_options {
        path = "/ping"
        port = var.monops_healthcheck_port
      }
    }
  }

  listener_spec {
    port        = 443
    target_port = var.monops_web_port
    name        = "monops-web"
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
      address     = ycp_vpc_address.monops_addr.ipv6_address.0.address
    }
  }
}


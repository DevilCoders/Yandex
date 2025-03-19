resource "ycp_load_balancer_network_load_balancer" "load_balancer" {
  name      = var.load_balancer_name
  region_id = var.region_id
  type      = "EXTERNAL"
  attached_target_group {
    target_group_id = ycp_load_balancer_target_group.target_group.id
    health_check {
      healthy_threshold   = 2
      interval            = "5s"
      name                = "ping"
      timeout             = "2s"
      unhealthy_threshold = 2
      http_options {
        path = "/ping"
        port = 80
      }
    }
  }
  listener_spec {
    port        = 443
    target_port = 443
    name        = var.load_balancer_name
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
      address     = var.ipv6_addr
    }
  }
}

resource "ycp_load_balancer_target_group" "target_group" {
  name      = var.load_balancer_name
  region_id = var.region_id

  dynamic "target" {
    for_each = var.targets

    content {
      address   = target.value.address
      subnet_id = target.value.subnet_id
    }
  }
}

terraform {
  required_version = ">= 0.14.7, < 0.15.0"
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">=0.32.0"
    }
  }
}

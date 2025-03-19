terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}

module "common" {
  source = "../../common"
}

resource "ycp_load_balancer_network_load_balancer" "load-balancer" {
  name      = "${var.lb_name_prefix}-${lower(var.ip_version)}-load-balancer"
  region_id = module.common.yc_region
  type      = "EXTERNAL"

  listener_spec {
    name        = "public-api-listener"
    port        = var.lb_public_api_port
    target_port = var.lb_public_api_port
    protocol = "TCP"
    external_address_spec {
      ip_version  = var.ip_version
      address     = var.ip_address
      yandex_only = var.yandex_only
    }
  }

  listener_spec {
    name        = "private-api-listener"
    port        = var.lb_private_api_port
    target_port = var.lb_private_api_port
    protocol = "TCP"
    external_address_spec {
      ip_version  = var.ip_version
      address     = var.ip_address
      yandex_only = var.yandex_only
    }
  }

  attached_target_group {
    target_group_id = ycp_load_balancer_target_group.target-group.id

    health_check {
      name                = "http-check"
      interval            = "5s"
      timeout             = "1s"
      healthy_threshold   = 3
      unhealthy_threshold = 3
      http_options {
        port = var.health_check_port
        path = var.health_check_path
      }
    }
  }
}

resource "ycp_load_balancer_target_group" "target-group" {
  name      = "${var.lb_name_prefix}-${lower(var.ip_version)}-target-group"
  region_id = module.common.yc_region

  dynamic "target" {
    for_each = var.subnets_addresses

    content {
      address   = target.value
      subnet_id = target.key
    }
  }

  labels = module.common.instance_labels
}

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

provider "ycp" {
  prod        = false
  ycp_profile = module.common.ycp_profile
  folder_id   = var.yc_folder
  zone        = module.common.yc_zone
}

resource "ycp_load_balancer_network_load_balancer" "load-balancer" {
  name      = "${var.lb_name_prefix}-${lower(var.ip_version)}-load-balancer"
  region_id = module.common.yc_region
  type      = "EXTERNAL"

  listener_spec {
    name        = "listener"
    port        = var.port
    target_port = var.port
    protocol    = "TCP"
    external_address_spec {
      ip_version  = var.ip_version
      address     = var.ip_address
      yandex_only = var.yandex_only
    }
  }

  attached_target_group {
    target_group_id = var.target_group_id

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


terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}

provider "ycp" {
  prod        = false
  ycp_profile = var.ycp_profile
  folder_id   = var.folder
  zone        = var.zone
}

resource "ycp_load_balancer_network_load_balancer" "load-balancer" {
  name      = var.name
  region_id = var.zone
  type      = "EXTERNAL"

  labels = {
    layer   = "paas"
    abc_svc = "yckms"
    env     = "pre-prod"
  }

  listener_spec {
    name        = "listener"
    port        = var.port
    target_port = var.port
    protocol    = "TCP"
    external_address_spec {
      address     = var.ip_address
      ip_version  = var.ip_version
      yandex_only = var.yandex_only
    }
  }

  attached_target_group {
    target_group_id = var.target_group_id

    health_check {
      name                = "healthcheck"
      interval            = "5s"
      timeout             = "1s"
      healthy_threshold   = 2
      unhealthy_threshold = 2
      http_options {
        path = var.health_check_path
        port = var.health_check_port
      }
    }
  }
}

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

resource "ycp_load_balancer_target_group" "target-group" {
  name      = var.name
  region_id = var.zone

  dynamic "target" {
    for_each = var.subnets_addresses

    content {
      address   = target.value
      subnet_id = target.key
    }
  }

  labels = {
    layer = "paas"
    abc_svc = "yckms"
    env = "pre-prod"
  }
}

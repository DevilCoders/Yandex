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

resource "ycp_load_balancer_target_group" "target-group" {
  name      = "${var.tg_name_prefix}-target-group"
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

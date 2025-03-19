terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
}

provider "ycp" {
  ycp_profile = var.ycp_profile
  folder_id   = var.yc_folder
  zone        = var.ycp_zone
}

terraform {
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">= 0.35" # optional
    }
  }
  required_version = "= 0.13.6"
}

terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">= 0.58, <= 0.58" # optional
    }
  }
  required_version = "= 0.13.6"
}

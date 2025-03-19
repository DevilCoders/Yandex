terraform {
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">= 0.58.0"
    }
  }

  required_version = ">= 0.13"
}

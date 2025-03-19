terraform {
  required_version = ">= 1.1.7"
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">=0.32.0"
    }
  }
}

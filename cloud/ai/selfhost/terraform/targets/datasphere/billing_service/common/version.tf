terraform {
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 1.0"
}

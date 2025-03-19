terraform {
  required_version = ">= 0.13"
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
    ytr = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ytr"
    }
  }
}

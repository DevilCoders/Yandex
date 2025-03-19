terraform {
  required_providers {
    yandex = {
      source = "terraform-registry.storage.yandexcloud.net/yandex-cloud/yandex"
    }
    random = {
      source = "terraform-registry.storage.yandexcloud.net/hashicorp/random"
    }
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = "0.54.0"
    }
  }
  required_version = ">= 0.13"
}
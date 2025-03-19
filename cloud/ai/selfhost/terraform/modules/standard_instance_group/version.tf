terraform {
  required_providers {
    yandex-public-tf-provider = {
      source = "terraform-registry.storage.yandexcloud.net/yandex-cloud/yandex"
    }
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 1.0"
}

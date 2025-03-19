terraform {
  required_providers {
    external = {
      source = "hashicorp/external"
      version = "2.2.0"
    }
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      # version = "0.20.0" # optional
    }
    yc = {
      source = "terraform-registry.storage.yandexcloud.net/yandex-cloud/yandex"
    }
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }
}

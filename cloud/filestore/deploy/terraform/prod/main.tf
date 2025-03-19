terraform {
  required_providers {
    yandex = {
      source  = "yandex-cloud/yandex"
      version = ">= 0.61.0"
    }
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">= 0.43.0"
    }
  }
  required_version = ">= 0.13"
}

provider "yandex" {
  endpoint    = "api.cloud.yandex.net:443"
  token       = var.token
  zone        = "ru-central1-c"
  max_retries = 10
}

provider "ycp" {
  token = var.token
  prod  = true // default value -> false
}

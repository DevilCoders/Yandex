terraform {
  required_version = ">= 0.13"
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = "0.60.0"
    }
    null = {
      source = "hashicorp/null"
    }
  }
}

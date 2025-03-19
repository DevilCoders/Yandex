terraform {
  required_version = "0.14.9"

  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      # version = "0.20.0" # optional
    }
  }
}

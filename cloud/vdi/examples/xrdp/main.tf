terraform {
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }
}

provider "yandex" {
  cloud_id  = var.cloud_id
  folder_id = var.folder_id
  zone      = var.zone
  token     = var.token
}

resource "random_string" "launch_code" {
  length  = 5
  special = false
  upper   = false
}

locals {
  name = join("-", ["xrdp", random_string.launch_code.result])
}

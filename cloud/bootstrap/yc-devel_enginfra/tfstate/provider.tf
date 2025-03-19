terraform {
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }
  required_version = ">= 0.13"
}

provider yandex {
  cloud_id = "yc-devel"
  folder_id = local.folder_id
  token = var.token
} 
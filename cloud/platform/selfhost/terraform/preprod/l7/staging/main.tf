terraform {
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">= 0.53"
    }
  }

  required_version = ">= 1"
}

provider "ycp" {
  prod      = false
  cloud_id  = "aoe0oie417gs45lue0h4"
  folder_id = "aoe4lof1sp0df92r6l8j"
}

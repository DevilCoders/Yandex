terraform {
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
}

module "vars" {
  source    = "./modules/vars"
  workspace = terraform.workspace
}

provider "yandex" {
  endpoint  = module.vars.yc.endpoint
  cloud_id  = module.vars.yc.cloud_id
  folder_id = module.vars.yc.folder_id
  zone      = module.vars.yc.zone
}

provider "ycp" {
  cloud_id  = module.vars.yc.cloud_id
  folder_id = module.vars.yc.folder_id
  zone      = module.vars.yc.zone
  prod      = module.vars.ycp.prod
}

data "yandex_resourcemanager_folder" "default" {
  folder_id = module.vars.yc.folder_id
}

locals {
  zones = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c"
  ]
}

terraform {
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }
  required_version = ">= 1.0"
}


provider "yandex" {
  service_account_key_file = var.service_account_key_file

  endpoint = module.constants.by_environment.yc_endpoint

  folder_id = var.folder_id
}
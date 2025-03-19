provider "yandex" {
  cloud_id                 = var.cloud_id
  folder_id                = var.folder_id
  service_account_key_file = var.service_account_key_file
}

provider "ycp" {
  cloud_id                 = var.cloud_id
  folder_id                = var.folder_id
  service_account_key_file = var.service_account_key_file
  ycp_config               = local.ycp_config
  ycp_profile              = var.ycp_profile
  prod                     = true
}

terraform {
  required_version = "0.14.7"
  required_providers {
    yandex = {
      source  = "yandex-cloud/yandex"
      version = "0.51.1"
    }
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = "0.22.0"
    }
    template = {
      source = "hashicorp/template"
    }
  }
  backend "s3" {
    bucket   = "yc-search-tf-state"
    key      = "prod.tfstate"
    endpoint = "storage.yandexcloud.net"
    region   = "ru-central1"

    skip_region_validation      = true
    skip_credentials_validation = true
  }
}

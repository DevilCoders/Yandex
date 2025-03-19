provider "yandex" {
  cloud_id                 = var.cloud_id
  folder_id                = var.folder_id
  service_account_key_file = var.service_account_key_file
  endpoint                 = "api.cloud-preprod.yandex.net:443"
  storage_endpoint         = "storage.cloud-preprod.yandex.net"
  storage_access_key       = var.s3_admin_access_key
  storage_secret_key       = var.s3_admin_secret_key
}

provider "ycp" {
  cloud_id                 = var.cloud_id
  folder_id                = var.folder_id
  service_account_key_file = var.service_account_key_file
  ycp_config               = local.ycp_config
  ycp_profile              = var.ycp_profile
  prod                     = false
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
    bucket   = "yc-search-preprod-tf-state"
    key      = "preprod.tfstate"
    endpoint = "storage.cloud-preprod.yandex.net"
    region   = "ru-central1"

    skip_region_validation      = true
    skip_credentials_validation = true
  }
}

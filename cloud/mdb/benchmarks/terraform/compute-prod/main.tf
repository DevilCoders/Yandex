provider "yandex" {
  cloud_id                 = var.cloud_id
  folder_id                = var.folder_id
  service_account_key_file = var.service_account_key_file
  storage_access_key       = var.s3_admin_access_key
  storage_secret_key       = var.s3_admin_secret_key
}

provider "ycp" {
  ycp_config  = local.ycp_config
  ycp_profile = var.ycp_profile
  cloud_id    = var.cloud_id
  folder_id   = var.folder_id
  prod        = true
}


terraform {
  required_version = "1.0.5"
  required_providers {
    yandex = {
      source  = "yandex-cloud/yandex"
      version = "0.61.0"
    }
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">=0.32.0"
    }
    template = {
      source = "hashicorp/template"
    }
  }
  backend "s3" {
    bucket   = "yc-mdb-tanks-tfstate"
    key      = "prod.tfstate"
    endpoint = "storage.yandexcloud.net"
    region   = "ru-central1"

    skip_region_validation      = true
    skip_credentials_validation = true
  }
}

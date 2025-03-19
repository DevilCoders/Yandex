// Configure the Yandex.Cloud provider
provider "yandex" {
  cloud_id                 = var.cloud_id
  folder_id                = var.folder_id
  endpoint                 = "api.cloud-preprod.yandex.net:443"
  storage_endpoint         = "storage.cloud-preprod.yandex.net"
  service_account_key_file = var.service_account_key_file
  storage_access_key       = var.s3_admin_access_key
  storage_secret_key       = var.s3_admin_secret_key
}

provider "ycp" {
  ycp_config  = local.ycp_config
  ycp_profile = var.ycp_profile
  cloud_id    = var.cloud_id
  folder_id   = var.folder_id
  prod        = false
}

provider "ycp" {
  alias       = "s3"
  ycp_config  = local.ycp_config
  ycp_profile = var.ycp_profile
  cloud_id    = var.s3.cloud_id
  folder_id   = var.s3.folder_id
  prod        = false
}

terraform {
  required_version = "0.14.11"
  required_providers {
    yandex = {
      source  = "yandex-cloud/yandex"
      version = "0.74.0"
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
    bucket   = "mdb-preprod-control-plane-tfstate"
    key      = "preprod"
    endpoint = "storage.cloud-preprod.yandex.net"
    region   = "ru-central1"

    skip_region_validation      = true
    skip_credentials_validation = true
  }
}

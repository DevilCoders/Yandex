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
  ycp_profile = "selfhost-profile"
  cloud_id    = var.cloud_id
  folder_id   = var.folder_id
  prod        = true
}

terraform {
  required_version = "<=1.1.6"
  required_providers {
    yandex = {
      source  = "yandex-cloud/yandex"
      version = " 0.69.0"
    }
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">=0.41.0"
    }
    helm = {
      source = "hashicorp/helm"
    }
    kubectl = {
      source = "gavinbunney/kubectl"
    }
    elasticstack = {
      source  = "elastic/elasticstack"
      version = "~> 0.1.0"
    }
  }

  backend "s3" {
    bucket   = "cloudbeaver-preprod-terraform-state"
    key      = "B6lPBJAjPzRyI8eT7JEV"
    endpoint = "storage.cloud-preprod.yandex.net"
    region   = "ru-central1"

    skip_region_validation      = true
    skip_credentials_validation = true
  }
}

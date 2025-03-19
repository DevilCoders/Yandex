provider "ycp" {
  ycp_profile              = var.ycp_profile
  ycp_config               = local.gpn_sa_config
  service_account_key_file = local.gpn_sa_file
  cloud_id                 = var.yc_cloud
  folder_id                = var.yc_folder
  prod                     = false
  storage_endpoint_url     = "storage.gpn.yandexcloud.net"
}

provider "ycp" {
  alias                    = "s3"
  ycp_profile              = var.ycp_profile
  ycp_config               = local.gpn_sa_config
  service_account_key_file = local.gpn_sa_file
  cloud_id                 = var.s3.cloud_id
  folder_id                = var.s3.folder_id
  prod                     = false
}

terraform {
  required_version = "0.14.11"
  required_providers {
    yandex = {
      source  = "yandex-cloud/yandex"
      version = "0.50.0"
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
    bucket   = "mdb-gpn-control-plane-tfstate"
    key      = "prod"
    endpoint = "storage.gpn.yandexcloud.net"
    region   = "us-east-1"

    skip_region_validation      = true
    skip_credentials_validation = true
  }
}

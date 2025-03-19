provider "ycp" {
  ycp_profile = "israel-s3"
  ycp_config  = "israel_ycp_config.yaml"
  cloud_id    = var.cloud_id
  folder_id   = var.installation.folder_id
  prod        = false
}

terraform {
  required_version = "0.14.11"
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">=0.72.0"
    }
    template = {
      source = "hashicorp/template"
    }
  }
  backend "s3" {
    bucket   = "yc-yc-s3-israel-tfstate"
    key      = "prod"
    endpoint = "s3.mds.yandex.net"
    region   = "us-east-1"

    skip_region_validation      = true
    skip_credentials_validation = true
  }
}

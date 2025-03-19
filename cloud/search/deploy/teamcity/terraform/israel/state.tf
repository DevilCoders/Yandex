#provider "yandex" {
#  cloud_id                 = var.cloud_id
#  folder_id                = var.folder_id
#  service_account_key_file = var.service_account_key_file
#  endpoint                 = "api.cloud-preprod.yandex.net:443"
#  storage_endpoint         = "storage.cloud-preprod.yandex.net"
#  storage_access_key       = var.s3_admin_access_key
#  storage_secret_key       = var.s3_admin_secret_key
#}

provider "ycp" {
  prod      = true
  folder_id = var.yc_folder
  zone      = var.yc_zone
  ycp_profile = "israel"
}

terraform {
#  required_version = "0.14.7"
  required_providers {
    yandex = {
      source  = "yandex-cloud/yandex"
    }
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">= 0.73"
    }
    template = {
      source = "hashicorp/template"
    }
#    required_version = ">= 0.13"
  }

  backend "s3" {
    endpoint   = "storage.cloudil.com"
    bucket     = "yc-search-tf-state"
    key        = "israel.tfstate"
    region     = "us-east-1"
    // Access is done via the sa-terraform service account from the yc-cloud-trail cloud.
    // secret_key is stored in yav.yandex-team.ru
    // https://yav.yandex-team.ru/secret/sec-01g2fvpyyxkqjbkv5pj5z1ej43/explore/versions) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    // access_key = "YCF08I05VH0nZmswZdsfxWxd6"
    access_key = "YCF08U7NS-spYdjhEe4zLa_uW"
#    secret_key = ""

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

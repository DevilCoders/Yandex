provider "yandex" {
  cloud_id                 = var.cloud_id
  folder_id                = var.folder_id
  token                    = var.yc_token
  service_account_key_file = var.service_account_key_file
}

provider "ycp" {
  cloud_id                 = var.cloud_id
  folder_id                = var.folder_id
  token                    = var.yc_token
  service_account_key_file = var.service_account_key_file
  prod                     = true
}

terraform {
  backend "s3" {
    bucket   = "mdb-prod-control-plane-tfstate"
    key      = "appendix"
    endpoint = "storage.yandexcloud.net"
    region   = "ru-central1"

    skip_region_validation      = true
    skip_credentials_validation = true
  }
}

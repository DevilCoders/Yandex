locals {
    module_path = path_relative_from_include()
    key_file = abspath("${get_terragrunt_dir()}/${local.module_path}/generated.yc_creds")
}

generate "prov"{
    path      = "generated.providers.tf"
    if_exists = "overwrite"
    contents  = <<EOF
terraform {
  required_providers {
    yandex = {
      source  = "yandex-cloud/yandex"
      version = ">= 0.64"
    }
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
}

variable "yc_folder" {
  default     = "yc.billing.service-folder"
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud-preprod.yandex.net:443"
}

provider "yandex" {
  endpoint  = var.yc_endpoint
  service_account_key_file = "${local.key_file}"
  folder_id = var.yc_folder
}

provider "ycp" {
  prod      = false
  service_account_key_file = "${local.key_file}"
  folder_id = var.yc_folder
}
EOF
}

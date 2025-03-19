provider "yandex" {
  endpoint  = local.yc_endpoint
  token     = local.yc_token
  folder_id = local.yc_folder
  zone      = local.yc_zone
  version   = ">= 0.47.0"
}

provider ycp {
  prod      = false
  token     = local.yc_token
  folder_id = local.yc_folder
  zone      = local.yc_zone
}


locals {
  yc_folder   = "aoelu1c2p31vl6c6urig"
  yc_zone     = "ru-central1-c"
  yc_endpoint = "api.cloud-preprod.yandex.net:443"
  yc_token    = var.yc_token != "" ? var.yc_token : module.yc_token.result
  yc_ig_sacc  = "bfb13urmi99105i7fkcn"
}

module "yc_token" {
  source = "../../../modules/yc-token"
}

variable "zones" {
  type = list(string)
  default = [
    "vla",
    "sas",
    "myt",
  ]
}


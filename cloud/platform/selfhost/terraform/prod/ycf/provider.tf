provider "yandex" {
  endpoint  = local.yc_endpoint
  token     = local.yc_token
  folder_id = local.yc_folder
  zone      = local.yc_zone
}

provider "ycp" {
  prod      = true
  token     = local.yc_token
  folder_id = local.yc_folder
  zone      = local.yc_zone
}

locals {
  yc_folder   = "b1gbbk5crr4n6tp7re9s"
  s3_folder   = "b1gre9336vmeu4h13cra"
  yc_zone     = "ru-central1-c"
  yc_endpoint = "api.cloud.yandex.net:443"
  yc_token    = var.yc_token != "" ? var.yc_token : module.yc_token.result
  yc_ig_sacc  = "aje90ppqftf2psmfdmoc"
}

module "yc_token" {
  source = "../../modules/yc-token"
}

variable "zones" {
  type    = list(string)
  default = [
    "vla",
    "sas",
    "myt",
  ]
}


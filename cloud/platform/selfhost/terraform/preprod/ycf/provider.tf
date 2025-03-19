provider "yandex" {
  endpoint  = local.yc_endpoint
  token     = local.yc_token
  folder_id = local.yc_folder
  zone      = local.yc_zone
}

provider "ycp" {
  prod      = false
  token     = local.yc_token
  folder_id = local.yc_folder
  zone      = local.yc_zone
}

locals {
  yc_folder   = "aoe7n27ri5h87bblte5l"
  s3_folder   = "aoe3k3vl3vl17sifsg1a"
  yc_zone     = "ru-central1-c"
  yc_endpoint = "api.cloud-preprod.yandex.net:443"
  yc_token    = var.yc_token != "" ? var.yc_token : module.yc_token.result
  yc_ig_sacc  = "bfbkk5if54u6qm319u0e"
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

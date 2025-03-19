module "env" {
  source    = "../dependencies"
  yc_folder = var.yc_folder
}

locals {
  sa          = module.env.sa.id
  nsdomain    = module.env.nsdomain
  dns_zone_id = module.env.dns_zone.id
}

variable "image_name" {}
variable "logging_id" {}

locals {
  canary_zone = "ru-central1-c"

  lockbox_canary_secret = "fc3g636cpvt9cncse1pu"
  canary_installation   = "preprod-canary"
}

# Select image
data "yandex_compute_image" "image" {
  name      = var.image_name
  folder_id = var.yc_folder
}

output "image" {
  value = {
    id     = data.yandex_compute_image.image.id
    family = data.yandex_compute_image.image.family
    name   = data.yandex_compute_image.image.name
  }
}

module "piper-canary" {
  source               = "../../modules/base_instance"
  installation         = local.canary_installation
  name                 = "piper"
  hostname_suffix      = "-canary"
  instance_description = "canary host with piper image"
  folder_id            = var.yc_folder
  oslogin              = true
  image_id             = data.yandex_compute_image.image.image_id
  subnet               = module.env.subnets[local.canary_zone].id
  zone                 = local.canary_zone
  service_account_id   = local.sa

  nsdomain      = local.nsdomain
  dns_zone_id   = local.dns_zone_id
  skip_underlay = false

  metadata = {
    lockbox-secret-id = "${local.lockbox_canary_secret}"
  }

  logging_group_id   = var.logging_id
  security_group_ids = []
}

module "piper-canary-juggler" {
  source               = "../../modules/juggler/piper"
  yandex_token         = var.yt_token
  hosts                = [module.piper-canary.fqdn]
  installation         = local.canary_installation
  enable_notifications = false
}

output "piper-canary" {
  value = module.piper-canary
}

output "piper-canary-checks" {
  value = module.piper-canary-juggler
}


module "downtime" {
  source      = "../../modules/juggler/downtime"
  fqdn        = module.piper-canary.fqdn
  instance_id = module.piper-canary.instance_id
}

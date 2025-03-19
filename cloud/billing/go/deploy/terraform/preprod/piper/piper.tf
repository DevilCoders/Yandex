locals {
  piper_zones = {
    "ru-central1-a" = 1
    "ru-central1-b" = 0
    "ru-central1-c" = 1
  }

  lockbox_piper_secret = "fc3s7jddjsff6d65it8m"
  installation         = "preprod"
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

module "piper" {
  for_each = {
    for obj in flatten([
      for zone, cnt in local.piper_zones : [
        for i in range(cnt) : {
          zone  = zone
          index = i + 1
        }
      ]
    ]) : "${obj.zone}::${obj.index}" => obj
  }

  source               = "../../modules/base_instance"
  installation         = local.installation
  name                 = "piper"
  instance_description = "piper data processing host"
  host_index           = each.value.index
  folder_id            = var.yc_folder
  # ssh-keys             = module.billing-abc-ssh.ssh-keys
  oslogin            = true
  image_id           = data.yandex_compute_image.image.image_id
  subnet             = module.env.subnets[each.value.zone].id
  zone               = each.value.zone
  service_account_id = local.sa

  nsdomain    = local.nsdomain
  dns_zone_id = local.dns_zone_id

  metadata = {
    lockbox-secret-id = "${local.lockbox_piper_secret}"
  }

  logging_group_id   = var.logging_id
  security_group_ids = []
}

module "piper-juggler" {
  source       = "../../modules/juggler/piper"
  yandex_token = var.yt_token
  hosts        = [for k, v in module.piper : v.fqdn]

  installation         = local.installation
  enable_notifications = true
}

module "downtime" {
  for_each = {
    for k, v in module.piper : v.fqdn => v
  }

  source      = "../../modules/juggler/downtime"
  fqdn        = each.value.fqdn
  instance_id = each.value.instance_id
}


output "piper-hosts" {
  value = module.piper
}

output "piper-checks" {
  value = module.piper-juggler
}

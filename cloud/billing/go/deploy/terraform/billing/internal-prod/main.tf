locals {
  env = "internal_prod"
}


module "common_juggler_checks" {
  source = "../../modules/juggler/billing/common"

  env                         = local.env
  hosts                       = tolist([var.cgroup])
  yandex_token                = var.yt_token
  checks_host                 = var.checks_host
  enabled_phone_notifications = var.enabled_phone_notifications
}

module "internal_juggler_checks" {
  source = "../../modules/juggler/billing/internal"

  env                         = local.env
  hosts                       = tolist([var.cgroup])
  yandex_token                = var.yt_token
  checks_host                 = var.checks_host
  enabled_phone_notifications = var.enabled_phone_notifications
}

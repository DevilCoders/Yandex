module "parts" {
  source        = "../../parts"
  system        = var.env
  env           = var.env
}

locals {
  base_tags    = var.enabled_phone_notifications ? module.parts.tags.notify : module.parts.tags.default
  default_tags = module.parts.tags.default
  base_flaps   = module.parts.flaps.short
  long_flaps   = module.parts.flaps.long
}

module "base_aggregator" {
  source = "../../aggregators"
}

locals {
  monrun_checks_notifications   = [for k, v in module.monrun_checks_notifications : v.check]

  solomon_quota_checks = [for k, v in module.solomon_quota_checks : v.check]

  solomon_alerts_checks = [for k, v in module.solomon_alerts_checks : v.check]
}

locals {
  deploy_mark = var.checks_host

  checks = concat(
    local.monrun_checks_notifications,
    local.solomon_quota_checks,
    local.solomon_alerts_checks,
  )
}

module "apply_checks" {
  source       = "../../apply"
  checks       = local.checks
  deploy_mark  = "${local.deploy_mark}_common"
  yandex_token = var.yandex_token
}

output "host" {
  value = var.checks_host
}

output "checks" {
  value = local.checks
}

locals {
  services = [
    "billing-balance-health",
    "billing-notification-health",
    "billing-laas-health",
    "billing-tracker-health",
    "billing-alive-rt-enricher-var-workers",
    "billing-without-sync-task",
    "billing-not-ticked-subscriptions",
    "billing-mkt",
    "billing-mkt-private",
    "billing-alive-mkt-workers",
    "logreader"
  ]

  monrun_only_tg_notifications = [
    "logreader"
  ]

  monrun_checks_special_flaps = {
    "billing-balance-health": local.long_flaps,
    "billing-notification-health": local.long_flaps,
    "billing-laas-health": local.long_flaps,
    "billing-tracker-health": local.long_flaps,
    "logreader": local.long_flaps,
  }
}

module "monrun_check_children" {
  source             = "../../children"
  services           = local.services
  hosts     = var.hosts
  host_type = var.hosts_type
}


module "monrun_checks" {
  for_each = toset(local.services)

  source             = "../../check"
  host               = var.checks_host
  service            = each.value
  children           = module.monrun_check_children.for_service[each.value]
  tags               = contains(local.monrun_only_tg_notifications, each.value) ? local.default_tags : local.base_tags
  flaps              = lookup(local.monrun_checks_special_flaps, each.value, local.base_flaps)
  aggregator         = module.base_aggregator.immediate_crit
}

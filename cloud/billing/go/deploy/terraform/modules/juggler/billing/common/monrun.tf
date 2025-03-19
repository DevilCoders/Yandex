locals {
  services = [
    "billing",
    "billing-private",
    "billing-worker",
    "billing-kikimr-health",
    "billing-access-service-health",
    "billing-alive-rt-resharder-workers",
    "billing-alive-rt-enricher-base-workers",
    "billing-alive-rt-presenter-workers",
    "billing-alive-solomon-workers",
    "metrics-collector-state",
    "ntp",
  ]

  monrun_checks_special_flaps = {
    "billing-kikimr-health": local.long_flaps,
    "billing-access-service-health": local.long_flaps,
    "ntp": local.long_flaps
  }
}

module "monrun_check_children" {
  source             = "../../children"
  services           = local.services
  hosts     = var.hosts
  host_type = var.hosts_type
}


module "monrun_checks_notifications" {
  for_each = toset(local.services)

  source             = "../../check"
  host               = var.checks_host
  service            = each.value
  children           = module.monrun_check_children.for_service[each.value]
  tags               = local.base_tags
  flaps              = lookup(local.monrun_checks_special_flaps, each.value, local.base_flaps)
  aggregator         = module.base_aggregator.immediate_crit
}

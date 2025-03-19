locals {
  solomon_alerts_services = [
    "billing-invalid-metrics",
  ]

  solomon_with_phone_notifications = toset([
    "billing-invalid-metrics",
  ])
}

module "solomon_alerts_checks" {
  for_each = toset(local.solomon_alerts_services)

  source             = "../../check"
  host               = var.checks_host
  service            = each.value
  tags               = contains(local.solomon_with_phone_notifications, each.value) ? local.base_tags : local.default_tags
  flaps              = local.base_flaps
  ttl                = 300
  aggregator         = module.base_aggregator.crit_if_any
}

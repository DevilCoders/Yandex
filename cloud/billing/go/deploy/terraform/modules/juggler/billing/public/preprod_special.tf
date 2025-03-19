locals {
  preprod_special_services = [
    "billing-e2e-ticker-cost",
    "billing-e2e-ticker-usage-qty",
    "billing-e2e-ticker-producers-count",
    "sandbox-task-upload-test-data-to-yt"
  ]
}

module "preprod_special_checks" {
  for_each = toset(var.env == "preprod" ? local.preprod_special_services : [])

  source             = "../../check"
  host               = var.checks_host
  service            = each.value
  tags               = local.default_tags
  flaps              = local.long_flaps
  ttl                = 5400
  aggregator         = module.base_aggregator.immediate_crit
}

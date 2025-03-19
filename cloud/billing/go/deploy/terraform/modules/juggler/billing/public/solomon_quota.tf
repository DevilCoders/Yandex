locals {
  solomon_quota_services = [
    "billing_myt_sys",
    "billing_sas_sys",
    "billing_vla_sys",
    "sys",
    "marketplace",
    "marketplace_private"
  ]
}


module "solomon_quota_checks" {
  for_each = toset(local.solomon_quota_services)

  source             = "../../check"
  host               = "yc_billing_solomon_quota"
  service            = "yc_billing_${var.env}_${each.value}"
  tags               = local.default_tags
  flaps              = local.base_flaps
  aggregator         = module.base_aggregator.immediate_crit
}


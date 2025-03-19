locals {
  solomon_quota_services = [
    "api",
    "console_api",
    "metrics_collector",
    "uploader",
    "worker"
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


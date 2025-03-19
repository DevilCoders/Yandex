locals {
  sandbox_services = [
    "sandbox-task-enrich-billing-accounts",
    "sandbox-task-aggregate-invalid-metrics",
    "sandbox-task-enrich-services",

    "sandbox-task-enrich-sales-name-table",
    "sandbox-task-enrich-labels-map",
    "sandbox-task-find-duplicate-metrics",
    "duplicate-metrics-in-yt",
    "sandbox-task-suspend-data-from-yt-to-s3",
    "sandbox-task-mock-clouds",
    "sandbox-task-aggregate-balance-reports",
  ]

  sandbox_checks_special_ttl = {
    "sandbox-task-find-duplicate-metrics": 14400,
    "duplicate-metrics-in-yt": 14400,
    "sandbox-task-suspend-data-from-yt-to-s3": 5400,
    "sandbox-task-aggregate-balance-reports": 10800,
  }

  sandbox_with_phone_notifications = [
    "sandbox-task-enrich-sales-name-table",
    "sandbox-task-enrich-billing-accounts",
    "sandbox-task-enrich-labels-map",
    "sandbox-task-enrich-services",
    "sandbox-task-aggregate-invalid-metrics",
    "sandbox-task-aggregate-balance-reports",
  ]

  sandbox_task_yt_export_aggregations_children = [
    "sandbox-task-aggregate-exported-billing-accounts",
    "sandbox-task-aggregate-exported-billing-accounts-balance",
    "sandbox-task-aggregate-exported-billing-accounts-history",
    "sandbox-task-aggregate-exported-service-instance-bindings",
    "sandbox-task-aggregate-exported-commited-use-discounts",
    "sandbox-task-aggregate-exported-commited-use-discount-templates",
    "sandbox-task-aggregate-exported-labels-map",
    "sandbox-task-aggregate-exported-monetary-grant-offers",
    "sandbox-task-aggregate-exported-monetary-grants",
    "sandbox-task-aggregate-exported-operations",
    "sandbox-task-aggregate-exported-publisher-accounts",
    "sandbox-task-aggregate-exported-schemas",
    "sandbox-task-aggregate-exported-services",
    "sandbox-task-aggregate-exported-skus",
    "sandbox-task-aggregate-exported-sku-overrides",
    "sandbox-task-aggregate-exported-subscriptions",
    "sandbox-task-aggregate-exported-transactions",
    "sandbox-task-aggregate-exported-var-incentives",
    "sandbox-task-aggregate-exported-volume-incentives",
    "sandbox-task-aggregate-exported-conversion-rates",
    "sandbox-task-aggregate-exported-support-subscriptions",
    "sandbox-task-aggregate-exported-support-templates",
    "sandbox-task-aggregate-exported-var-adjustments",
    "sandbox-task-aggregate-exported-balance-reports"
  ]
}

module "sandbox_checks" {
  for_each = toset(local.sandbox_services)

  source             = "../../check"
  host               = var.checks_host
  service            = each.value
  tags               = contains(local.sandbox_with_phone_notifications, each.value) ? local.base_tags : local.default_tags
  flaps              = local.long_flaps
  ttl                = lookup(local.sandbox_checks_special_ttl, each.value, 3600)
  aggregator         = module.base_aggregator.immediate_crit
}

module "sandbox_check_task_yt_export_aggregations" {
  source             = "../../check"
  host               = var.checks_host
  service            = "sandbox-task-yt-export-aggregations"
  children           = [
    for service in local.sandbox_task_yt_export_aggregations_children : {
      host       = var.checks_host
      service    = service
      group_type = "HOST"
    }
  ]
  tags               = local.base_tags
  flaps              = local.long_flaps
  ttl                = 5400
  aggregator         = module.base_aggregator.immediate_crit
}

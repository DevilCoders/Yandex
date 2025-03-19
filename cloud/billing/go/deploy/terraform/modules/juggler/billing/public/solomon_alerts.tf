locals {
  solomon_alerts_services = [
    "billing-yt-lb-raw",
    "billing-transactions-frozen",
    "billing-accounts-sudden-change",
    "billing-logbroker-write-quota",
    "billing-logfeller-export",
    "services-incorrect-lb-dc",
    "billing-clickhouse-health-free-disk-size",
    "billing-console-api-errors",
    "billing-clickhouse-health-replicas-available-for-write",
    "billing-clickhouse-health-replicas-available-for-read",
    "billing-fraud-client-errors",
    "broken-repository-indexes",
    "billing-realtime-incorrect-lb-dc",
    "billing-realtime-logfeller-unparsed",
    "yc-billing-public-api-gateway-error",
    "billing-failed-employee-grant-programs",
    "billing-kz-personal-data-export",
    "billing-kz-personal-data-export-errors",
  ]

  solomon_alerts_special_flaps = {
    "billing-fraud-client-errors": {
      critical_time = 1800
      stable_time   = 900
    },
  }

  solomon_with_phone_notifications = [
    "billing-logbroker-write-quota",
    "billing-clickhouse-health-free-disk-size",
    "billing-console-api-errors",
    "billing-clickhouse-health-replicas-available-for-write",
    "billing-clickhouse-health-replicas-available-for-read",
    "broken-repository-indexes",
    "billing-realtime-incorrect-lb-dc",
    "yc-billing-public-api-gateway-error",
    "marketplace-metering-api",
    "billing-failed-employee-grant-programs",
  ]

  solomon_alerts_check_invalid_metrics_children = [
    var.checks_host,
    "${var.checks_host}_piper",
  ]
}

module "solomon_alerts_checks" {
  for_each = toset(local.solomon_alerts_services)

  source             = "../../check"
  host               = var.checks_host
  service            = each.value
  tags               = contains(local.solomon_with_phone_notifications, each.value) ? local.base_tags : local.default_tags
  flaps              = lookup(local.solomon_alerts_special_flaps, each.value, local.base_flaps)
  ttl                = 300
  aggregator         = module.base_aggregator.crit_if_any
}

module "solomon_alerts_check_invalid_metrics" {
  source             = "../../check"
  host               = var.checks_host
  service            = "billing-invalid-metrics"
  children           = [
  for child_host in local.solomon_alerts_check_invalid_metrics_children : {
    host       = child_host
    service    = "billing-invalid-metrics"
    group_type = "HOST"
  }
  ]
  tags               = local.base_tags
  flaps              = local.base_flaps
  ttl                = 300
  aggregator         = module.base_aggregator.crit_if_any
}

locals {
  sandbox_services = [
    "sandbox-task-enrich-billing-accounts",
    "sandbox-task-enrich-services",
  ]

  sandbox_task_yt_export_aggregations_children = [
    "sandbox-task-aggregate-exported-billing-accounts",
    "sandbox-task-aggregate-exported-services",
    "sandbox-task-aggregate-exported-skus"
  ]
}

module "prod_special_sandbox_checks" {
  for_each = toset(var.env == "internal_prod" ? local.sandbox_services : [])

  source             = "../../check"
  host               = var.checks_host
  service            = each.value
  tags               = local.base_tags
  flaps              = local.long_flaps
  ttl                = 3600
  aggregator         = module.base_aggregator.immediate_crit
}

module "prod_special_sandbox_check_task_yt_export_aggregations" {
  for_each = toset(var.env == "internal_prod" ? ["sandbox-task-yt-export-aggregations"] : [])

  source             = "../../check"
  host               = var.checks_host
  service            = each.value
  tags               = local.base_tags
  flaps              = local.long_flaps
  ttl                = 5400
  children           = [
    for service in local.sandbox_task_yt_export_aggregations_children : {
      host       = var.checks_host
      service    = service
      group_type = "HOST"
    }
  ]
  aggregator         = module.base_aggregator.immediate_crit
}

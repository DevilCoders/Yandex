locals {
  prod_special_services = [
    "sandbox-task-get-user-sessions_v2",
    "sandbox-task-group-by-revenue-reports",
  ]

  sandbox_prod_checks_special_ttl = {
    "sandbox-task-group-by-revenue-reports" : 5400,
    "sandbox-task-get-user-sessions_v2" : 172800
  }
}

module "prod_special_checks" {
  for_each = toset(var.env == "prod" ? local.prod_special_services : [])

  source             = "../../check"
  host               = var.checks_host
  service            = each.value
  tags               = local.base_tags
  flaps              = local.base_flaps
  ttl                = local.sandbox_prod_checks_special_ttl[each.value]
  aggregator         = module.base_aggregator.immediate_crit
}

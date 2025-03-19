locals {
  services = [
    "push-client",
    "logsync",
  ]
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
  tags               = local.base_tags
  flaps              = local.base_flaps
  aggregator         = module.base_aggregator.immediate_crit
}

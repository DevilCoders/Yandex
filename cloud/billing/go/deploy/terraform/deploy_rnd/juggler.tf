locals {
  juggler_env        = "preprod"
  juggler_system     = "paas-canary"
  juggler_hosts      = [module.pass-base-canary.fqdn]
  juggler_instances  = [module.pass-base-canary]
  juggler_check_host = "yc_billing_rnd"
  juggler_service    = "freespace"
}


module "jp" {
  source = "../modules/juggler/parts"
  env    = local.juggler_env
  system = local.juggler_system
}

module "ja" {
  source = "../modules/juggler/aggregators"
}

module "children" {
  source         = "../modules/juggler/children"
  services       = [local.juggler_service]
  hosts = local.juggler_hosts
}

module "checks" {
  source         = "../modules/juggler/check"
  host           = local.juggler_check_host
  service        = local.juggler_service
  children       = module.children.for_service[local.juggler_service]
  tags           = module.jp.tags.default
  flaps          = module.jp.flaps.short
  aggregator     = module.ja.immediate_crit
}

module "downtime" {
  for_each = {
    for obj in local.juggler_instances : obj.fqdn => obj
  }

  source      = "../modules/juggler/downtime"
  fqdn        = module.pass-base-canary.fqdn
  instance_id = module.pass-base-canary.instance_id
}

resource "ytr_juggler_aggregate" "yc_billing_rnd-freespace" {
  namespace = module.checks.ytr.namespace
  host      = module.checks.ytr.host
  service   = module.checks.ytr.service
  raw_yaml  = module.checks.ytr.raw_yaml
}

output "check" {
  value = {
    id      = resource.ytr_juggler_aggregate.yc_billing_rnd-freespace.id
    host    = resource.ytr_juggler_aggregate.yc_billing_rnd-freespace.host
    service = resource.ytr_juggler_aggregate.yc_billing_rnd-freespace.service
  }
}

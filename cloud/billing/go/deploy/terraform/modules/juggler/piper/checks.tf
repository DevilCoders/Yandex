variable "checks_env_mapping" {
  type = map(string)
  default = {
    "prod"        = "prod"
    "prod-canary" = "prod"
  }
}

locals {
  checks_env = lookup(var.checks_env_mapping, var.installation, "preprod")
}

module "parts" {
  source = "../parts"
  system = "piper"
  env    = local.checks_env
}

locals {
  check_host = "yc_billing_piper_${var.installation}"

  base_tags = var.enable_notifications ? tolist(module.parts.tags.notify) : tolist(module.parts.tags.default)
  flaps     = module.parts.flaps.long

  alive_service = "passive-check-deliver"
}

module "host_aggregator" {
  source = "../aggregators"
}

module "host_children" {
  source   = "../children"
  services = [local.alive_service]
  hosts    = var.hosts
}

module "host_alive" {
  source     = "../check"
  host       = local.check_host
  service    = "host-alive"
  children   = module.host_children.for_service[local.alive_service]
  tags       = local.base_tags
  flaps      = local.flaps
  aggregator = module.host_aggregator.immediate_crit
}

module "pod_aggregator" {
  source               = "../aggregators"
  skip_unreach_service = "host-alive"
}

module "pod_children" {
  source = "../children"
  services = [
    "piper-pod", "piper-pod-piper", "piper-pod-unified-agent", "piper-pod-jaeger-agent", "piper-pod-tvmtool"
  ]
  hosts = var.hosts
}

module "pod_alive" {
  source     = "../check"
  host       = local.check_host
  service    = "piper-pod"
  children   = module.pod_children.for_service["piper-pod"]
  tags       = local.base_tags
  flaps      = local.flaps
  aggregator = module.pod_aggregator.immediate_crit
}

module "pod_containers" {
  for_each = toset(["piper", "unified-agent", "jaeger-agent", "tvmtool"])

  source     = "../check"
  host       = local.check_host
  service    = "piper-pod-${each.value}"
  children   = module.pod_children.for_service["piper-pod-${each.value}"]
  tags       = local.base_tags
  flaps      = local.flaps
  aggregator = module.pod_aggregator.immediate_crit
}

module "piper_health_aggregator" {
  source               = "../aggregators"
  skip_unreach_service = "piper-pod-piper"
}

module "health_children" {
  source = "../children"
  services = [
    "piper-health", "piper-health-ydb", "piper-health-iam", "piper-health-iam-meta", "piper-health-resharder-lbreader"
  ]
  hosts = var.hosts
}

module "piper_health" {
  source     = "../check"
  host       = local.check_host
  service    = "piper-health"
  children   = module.health_children.for_service["piper-health"]
  tags       = local.base_tags
  flaps      = local.flaps
  aggregator = module.piper_health_aggregator.immediate_crit
}

module "piper_services" {
  for_each = toset(["ydb", "iam", "iam-meta", "resharder-lbreader"])

  source     = "../check"
  host       = local.check_host
  service    = "piper-health-${each.value}"
  children   = module.health_children.for_service["piper-health-${each.value}"]
  tags       = local.base_tags
  flaps      = local.flaps
  aggregator = module.piper_health_aggregator.immediate_crit
}

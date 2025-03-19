
locals {
  deploy_mark = local.check_host

  checks = concat([
    module.host_alive.check,
    module.pod_alive.check,
    module.piper_health.check,
    ],
    [for k, v in module.pod_containers : v.check],
    [for k, v in module.piper_services : v.check],
  )
}

module "apply_checks" {
  source       = "../apply"
  checks       = local.checks
  deploy_mark  = local.deploy_mark
  yandex_token = var.yandex_token
}

output "host" {
  value = local.check_host
}

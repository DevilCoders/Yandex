locals {
  repositories = {
    preprod = "registry.yandex.net/cloud/api/metadata"
    staging = "registry.yandex.net/cloud/api/metadata"
    prod    = "registry.yandex.net/cloud/api/metadata"
  }

  latest_tag = "1b931b4a1"

  tag = coalesce(var.tag, local.latest_tag)
}

module "configs_deployer_container" {
  source = "../../modules/container"

  name    = "deploy-configs"
  is_init = true

  environment  = var.environment
  repositories = local.repositories
  tag          = local.tag

  mounts = {
    all-configs = {
      path      = "/etc"
      read_only = false
    }
  }
}
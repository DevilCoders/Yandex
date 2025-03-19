locals {
  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/unified-agent"
    staging = "cr.yandex/crppns4pq490jrka0sth/unified-agent"
    prod    = "cr.yandex/crppns4pq490jrka0sth/unified-agent"
  }

  latest_tag = "0.1"
  tag        = coalesce(var.tag, local.latest_tag)
}

module "unified_agent_container" {
  source = "../../modules/container"

  name         = "unified-agent"
  environment  = var.environment
  repositories = local.repositories
  tag          = local.tag

  ports = [
    16399
  ]

  // TODO: This is temporal solution
  envvar = [
    "TVM_SECRET"
  ]

  mounts = {
    # Looking for /etc/yandex/unified_agent/config.yml
    unified-agent-config = {
      path      = "/etc/yandex/unified_agent"
      read_only = true
    }
    unified-agent-data = {
      path                          = "/data"
      terminationGracePeriodSeconds = 30
      read_only                     = false
    }
    etc-secrets = {
      path      = "/etc/secrets"
      read_only = true
    }
  }
}

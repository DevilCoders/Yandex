locals {
  repositories = {
    preprod = "registry.yandex.net/cloud-ai/solomon-agent"
    staging = "registry.yandex.net/cloud-ai/solomon-agent"
    prod    = "registry.yandex.net/cloud-ai/solomon-agent"
  }

  latest_tag = "8218251"
  tag        = coalesce(var.tag, local.latest_tag)
}

module "solomon_agent_container" {
  source = "../../modules/container"

  name         = "solomon-agent"
  environment  = var.environment
  repositories = local.repositories
  tag          = local.tag

  mounts = {
    solomon-agent-config = {
      path      = "/etc/solomon-agent/"
      read_only = true
    }

    # TODO: is secrets really required?
    # etc-secrets = {
    #   path      = "/etc/secrets"
    #   read_only = true
    # }
  }

  command = [
    "/bin/bash"
  ]

  args = [
    "-c",
    "/usr/local/bin/solomon-agent --config /etc/solomon-agent/agent.json"
  ]
}
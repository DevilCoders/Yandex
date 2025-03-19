module "solomon_agent_component" {
  source = "../../../../components/solomon_agent"

  environment = var.environment

  // TODO: currently we don't have dedicated provider for registration for infra services
  //       datasphere provider will be used instead for this
  # solomon_provider = "infra"
  solomon_provider = "datasphere"

  solomon_cluster = var.environment == "preprod" ? "PREPROD" : "PROD"

  push_sources = [
    {
      name         = "PushModule"
      bind_address = "::"
      bind_port    = 8003
      handlers = [
        {
          project  = "cloud_ai"
          service  = "ai_zk"
          endpoint = "/"
        }
      ]
    }
  ]

  system_sources = [
    {
      project       = "cloud_ai"
      service       = "system"
      pull_interval = "15s"
      level         = "BASIC"
      additional_labels = {
        name = "system"
      }
    }
  ]
}

module "logrotate_component" {
  source = "../../../../components/logrotate"

  environment = var.environment
}

module "push_client_component" {
  source = "../../../../components/push_client"

  environment = var.environment

  files = [
    {
      path  = "/var/log/zk.log"
      topic = module.constants.by_environment.topic_logs_common

      # TODO: Add additional option
      # send_delay = 30
    }
  ]
}

module "cadvisor_component" {
  source = "../../../../components/cadvisor"

  environment = var.environment
}


module "configs_deployer_component" {
  source = "../../../../components/configs_deployer"

  environment = var.environment
}
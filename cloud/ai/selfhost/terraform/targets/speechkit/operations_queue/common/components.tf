module "solomon_agent_component" {
  source = "../../../../components/solomon_agent"

  environment = var.environment

  solomon_provider = "datasphere"
  solomon_cluster  = var.environment == "preprod" ? "PREPROD" : "PROD"

  push_sources = [
    {
      name         = "PushModule"
      bind_address = "::"
      bind_port    = 8003
      handlers = [
        {
          project  = "cloud_ai"
          service  = "operations_queue"
          endpoint = "/"
        }
      ]
    }
  ]

  system_sources = [
    {
      project           = "cloud_ai"
      service           = "operations_queue"
      pull_interval     = "15s"
      level             = "BASIC"
      additional_labels = {}
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
      path  = "/var/log/yc/ai/services-proxy.log"
      topic = module.constants.by_environment.topic_logs_operations_queue

      # TODO: Add additional option
      # send_delay = 30
    }
  ]
}

module "push_client_billing_component" {
  source = "../../../../components/push_client"

  environment = var.environment

  files = [
    {
      path  = "/var/lib/billing/accounting.log"
      topic = module.constants.by_environment.topic_billing

      # TODO: Add additional option
      # send_delay = 30
    }
  ]

  config_file = "push-client-billing.yaml"
}

module "cadvisor_component" {
  source = "../../../../components/cadvisor"

  environment = var.environment
}


module "configs_deployer_component" {
  source = "../../../../components/configs_deployer"

  environment = var.environment
}
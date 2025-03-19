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
          service  = "node_deployer"
          endpoint = "/"
        }
      ]
    }
  ]

  system_sources = [
    {
      project           = "cloud_ai"
      service           = "node_deployer"
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
      path  = "/var/log/ai/ml/node-deployer/debug.log"
      topic = module.constants.by_environment.topic_logs_node_deployer

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

module "unified_agent_component" {
  source = "../../../../components/unified_agent"

  environment = var.environment

  wellknown_logs_routes = [
    "yc_billing_ml"
  ]

  tvm_secret = var.tvm_secret
}

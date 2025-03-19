module "logrotate_component" {
  source = "../../../../components/logrotate"

  environment = var.environment

  additional_files = [
    "/var/log/envoy/access.log",
    "/var/log/envoy/process.log"
  ]
}

module "push_client_component" {
  source = "../../../../components/push_client"

  environment = var.environment

  files = [
    {
      path  = "/var/log/envoy/access.log"
      topic = module.constants.by_environment.topic_logs_node_proxy_access

      # TODO: Add additional field
      # send_delay = 30
    },
    {
      path  = "/var/log/envoy/process.log"
      topic = module.constants.by_environment.topic_logs_common

      # TODO: Add additional field
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

locals {
  solomon_installation = var.environment == "preprod" ? "cloud_preprod" : "cloud_prod"
  solomon_project      = var.environment == "preprod" ? "cloud_ai" : "datasphere"
  solomon_service      = "node-proxy"
}

module "unified_agent_component" {
  source = "../../../../components/unified_agent"

  environment = var.environment

  wellknown_logs_routes = [
    "yc_billing_ml"
  ]

  tvm_secret = var.tvm_secret

  custom_metrics_routes = [
    # envoy metrics
    {
      input = {
        plugin = "metrics_pull"
        config = {
          url = "http://localhost:9901/stats/prometheus"
          format = {
            prometheus = {}
          }
          project = local.solomon_project
          service = local.solomon_service
          cluster = var.environment
        }
      }
      channel = {
        output = {
          plugin = "metrics_multishard_pull"
          config = {
            port = 8005
            reshard_metrics = {
              project = local.solomon_project
              service = local.solomon_service
              cluster = var.environment
            }
            registration = {
              service_provider = "datasphere"
              endpoints = [
                {
                  installation = local.solomon_installation
                  iam = {
                    cloud_meta = {}
                  }
                }
              ]
            }
          }
        }
      }
    }
  ]
}
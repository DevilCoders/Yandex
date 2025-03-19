# module "solomon_agent_component" {
#   source = "../../../../components/solomon_agent"

#   environment = var.environment

#   solomon_provider = "datasphere"
#   solomon_cluster  = var.environment == "preprod" ? "PREPROD" : "PROD"

#   pull_sources = [
#     {
#       project           = "cloud_ai"
#       service           = "datasphere"
#       pull_interval     = "15s"
#       url               = "http://127.0.0.1:6789"
#       format            = "PROMETHEUS"
#       additional_labels = {}
#     }
#   ]

#   system_sources = [
#     {
#       project       = "cloud_ai"
#       service       = "system"
#       pull_interval = "15s"
#       level         = "BASIC"
#       additional_labels = {
#         name = "system"
#       }
#     }
#   ]

#   shards = [
#     {
#       project  = "cloud_ai"
#       service  = "datasphere"
#       preserve = true

#       override = {
#         project = "{{cloud_id}}"
#         cluster = "{{folder_id}}"
#         service = "datasphere"
#       }
#     }
#   ]
# }

module "logrotate_component" {
  source = "../../../../components/logrotate"

  environment = var.environment

  additional_files = [
    "/var/log/envoy-xds/process.log"
  ]
}

module "push_client_component" {
  source = "../../../../components/push_client"

  environment = var.environment

  files = [
    {
      # TODO: Replace with correct log file (rebuild image)
      path = "/var/log/envoy-xds/process.log"
      # send_delay = 30
      topic = module.constants.by_environment.topic_logs_node_deployer
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
  solomon_service      = "node-proxy-xds"
}

module "unified_agent_component" {
  source = "../../../../components/unified_agent"

  environment = var.environment

  tvm_secret = var.tvm_secret

  custom_metrics_routes = [
    # envoy metrics
    {
      input = {
        plugin = "metrics_pull"
        config = {
          url = "http://localhost:6789"
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
              project = "{{cloud_id}}"
              cluster = "{{folder_id}}"
              service = "datasphere"
              preserve_original = true
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
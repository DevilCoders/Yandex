locals {

  yc_logging_route = {
    input = {
      plugin = "grpc"
      config = {
        uri = "localhost:16400"
      }
    }
    channel = {
      pipe = [
        {
          storage_ref = {
            name = "main"
          }
        }
      ]

      output = {
        plugin = "logbroker"
        config = {
          endpoint = "lb.etn03iai600jur7pipla.ydb.mdb.yandexcloud.net"
          port = 2135
          topic = "/yc.logs.cloud/yc-logging"
          database = "/global/b1gvcqr959dbmi1jltep/etn03iai600jur7pipla"
          iam = {
            cloud_meta = {}
          }
          use_ssl = {}
          use_ydb_discovery = true
          codec = "zstd"
        }
      }
    }
  }
  
  tvm_client_id_by_environment = {
    preprod = 2019471,
    staging = 2019469,
    prod = 2019469
  }

  tvm_destination_id_by_environment = {
    preprod = 2001059
    staging = 2001059
    prod = 2001059
  }

  yc_billing_route_ml = {
    input = {
      plugin = "grpc"
      config = {
        uri = "localhost:16399"
      }
    }

    channel = {
      pipe = [
        {
          storage_ref = {
            name = "main"
          }
        }
      ]

      output = {
        plugin = "logbroker"
        config = {
          endpoint = "lb.etn03iai600jur7pipla.ydb.mdb.yandexcloud.net"
          port = 2135
          topic = "yc.billing.service-cloud/billing-ml-platform"
          database = "/global/b1gvcqr959dbmi1jltep/etn03iai600jur7pipla"
          iam = {
            jwt = {
              file = "/etc/secrets/logbroker_key.json"
              endpoint = "iam.api.cloud.yandex.net"
            }
          }
          use_ssl = {}
          use_ydb_discovery = true
        }
      }
    }
  }

  wellknown_logs_routes = {
    yc_logging = local.yc_logging_route
    yc_billing_ml = local.yc_billing_route_ml
  }



  wellknown_metrics_routes = {

  }


  used_wellknown_logs_routes = [
    for key in var.wellknown_logs_routes:
      local.wellknown_logs_routes[key]
  ]

  used_wellknown_metrics_routes = [
    for key in var.wellknown_metrics_routes:
      local.wellknown_metrics_routes[key]
  ]

  /*
   * Representation of Unified agent configuration
   * https://docs.yandex-team.ru/unified_agent/configuration
   */
  unified_agent_conf = {
    status = {
      port = 16301
    }
    /*
     * TODO: Investigate is more than one storage is required for persisting logs?
     *       Maybe separate storage for critical data (e.g. billing logs)
     */
    storages = [
      {
        name = "main"
        plugin = "fs"
        config = {
          directory = "./data/storage"
          max_partition_size = "10gb"
        }
      }
    ]

    routes = concat(
      local.used_wellknown_logs_routes,
      var.custom_logs_routes,
      local.used_wellknown_metrics_routes,
      var.custom_metrics_routes,
    )
  }

  configs = {
    "/etc/yandex/unified_agent/config.yml" = yamlencode(local.unified_agent_conf)
  }
}
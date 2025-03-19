locals {

  endpoint_type_by_environment = {
    preprod = "CLOUD_PREPROD"
    staging = "CLOUD_PROD"
    prod    = "CLOUD_PROD"
  }

  logger_conf = {
    LogTo = "STDERR"
    Level = "DEBUG"
  }

  storage_conf = {
    # determines how much historical data is held in memory if a fetcher lags behind
    # measured in chunks which roughly represent the data produced by one
    # module in one call
    BufferSize = 2048
  }

  auth_config = {
    ThreadPoolName = "Default"
    AuthMethods = [
      {
        Name = "iam_metadata"
        MetadataServiceConfig = {
          InstanceType = "OVERLAY"
        }
      }
    ]
  }

  registration_conf = {
    Provider     = var.solomon_provider # "datasphere"
    PullInterval = "15s"
    Endpoints = [{
      Type       = local.endpoint_type_by_environment[var.environment]
      AuthMethod = "iam_metadata"
    }]
  }

  cluster_conf = var.solomon_cluster

  shards = [
    for shard in var.shards :
    {
      Project          = shard.project  # "${var.project}"       # "cloud_ai"
      Service          = shard.service  # "${var.shard_service}" # "datasphere"
      PreserveOriginal = shard.preserve # also send data without overrides
      ShardKeyOverride = {
        Project : shard.override.project #"{{cloud_id}}"
        Cluster : shard.override.cluster # "{{folder_id}}"
        Service : shard.override.service # "${var.shard_service}" # "datasphere"
      }
    }
  ]

  http_server_conf = {
    BindAddress      = "::"
    BindPort         = 8004
    MaxConnections   = 100
    OutputBufferSize = 256
    ThreadsCount     = 4
    MaxQueueSize     = 200
    Shards           = local.shards
  }

  push_sources = [
    for source in var.push_sources :
    {
      Name        = source.name         # "PushModule"
      BindAddress = source.bind_address # "::"
      BindPort    = source.bind_port    # 8003
      ThreadCount = 4

      Handlers = [
        for handler in source.handlers :
        {
          Project  = handler.project
          Service  = handler.service
          Endpoint = handler.endpoint
        }
      ]
    }
  ]

  # {
  #               # mandatory labels
  #               Project: "${project}"
  #               Service: "${service}"
  #               Endpoint: "/proxy"
  #           },
  #           {
  #               # mandatory labels
  #               Project: "${project}"
  #               Service: "${shard_service}"
  #               Endpoint: "/cloud"
  #           }

  pull_sources = [
    for source in var.pull_sources :
    {
      Project = source.project # ${var.project}"       # "cloud_ai"
      Service = source.service # "${var.shard_service}" # "datasphere"
      Labels = [
        for k, v in source.additional_labels :
        "${k}=${v}"
        # "name=system"
      ]
      # how often to pull data
      PullInterval = source.pull_interval # "15s"

      Modules = [
        {
          HttpPull = {
            Url    = source.url    # "http://127.0.0.1:6789"
            Format = source.format # "PROMETHEUS"
          }
        }
      ]
    }
  ]

  system_sources = [
    for source in var.system_sources :
    {
      Project = source.project # "${var.project}" # "cloud_ai"
      Service = source.service # "system"         # "datasphere"
      Labels = [
        for k, v in source.additional_labels :
        "${k}=${v}"
        # "name=system"
      ]
      # how often to pull data
      PullInterval = source.pull_interval #"15s"

      Modules = [{
        System = {
          Cpu     = source.level # "BASIC"
          Memory  = source.level # "BASIC"
          Network = source.level # "BASIC"
          Storage = source.level # "BASIC"
          Io      = source.level # "BASIC"
          Kernel  = source.level # "BASIC"
        }
      }]
    }
  ]

  /*
   * Representation of Solomon configuration for multishard pull
   * https://wiki.yandex-team.ru/solomon/agent/service-providers
   * Important fields in configuration
   *   - Project :: top-level entity almost always will be cloud_ai for our servicies
   *     https://solomon.cloud.yandex-team.ru/?project=cloud_ai
   *   - Cluster :: entity used to specify environment from which metric origin itself
   *     valid values - TODO: TBD
   *   - Service :: entity representing source service of the metric
   */
  solomon_agent_conf = {
    Logger = local.logger_conf

    Storage = local.storage_conf

    AuthProvider = local.auth_config

    Registration = local.registration_conf

    Cluster = local.cluster_conf

    HttpServer = local.http_server_conf

    ConfigLoader = {
      Static = {
        Services = concat(local.pull_sources, local.system_sources)
        # [
        #   {
        #     Project = "${var.project}"       # "cloud_ai"
        #     Service = "${var.shard_service}" # "datasphere"
        #     # how often to pull data
        #     PullInterval = "15s"

        #     Modules = [{
        #       HttpPull = {
        #         Url    = "http://127.0.0.1:6789"
        #         Format = "PROMETHEUS"
        #       }
        #     }]
        #   },
        #   {
        #     Project = "${var.project}" # "cloud_ai"
        #     Service = "system"         # "datasphere"
        #     Labels = [
        #       "name=system"
        #     ]
        #     # how often to pull data
        #     PullInterval = "15s"

        #     Modules = [{
        #       System = {
        #         Cpu     = "BASIC"
        #         Memory  = "BASIC"
        #         Network = "BASIC"
        #         Storage = "BASIC"
        #         Io      = "BASIC"
        #         Kernel  = "BASIC"
        #       }
        #     }]
        #   }
        # ]
      }
    }


    Modules = [
      for source in local.push_sources :
      {
        HttpPush = source
      }
    ]

  }

  configs = {
    "/etc/solomon-agent/agent.json" = jsonencode(local.solomon_agent_conf)
  }
}
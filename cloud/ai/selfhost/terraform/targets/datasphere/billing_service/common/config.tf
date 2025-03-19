locals {
  dburl_by_environment = {
    # preprod = "jdbc:postgresql://rc1a-uw2lefs6js8kdqtb.mdb.cloud-preprod.yandex.net:6432/db1?&targetServerType=master&ssl=true&sslmode=verify-full"
    # staging = "jdbc:postgresql://rc1b-tr7shbe01pwg2t8e.mdb.yandexcloud.net:6432/db1?&targetServerType=master&ssl=true&sslmode=verify-full"
    prod    = "jdbc:postgresql://rc1a-7j6pl2qrqkb6gfyq.mdb.yandexcloud.net:6432/ds_billing_db?&targetServerType=master&ssl=true&sslmode=verify-full"
  }

  // FIXME: read this variables from Vault
  s3_endpoint_by_environment = {
    preprod = "storage.cloud-preprod.yandex.net"
    staging = "storage.yandexcloud.net"
    prod    = "storage.yandexcloud.net"
  }

  sg_endpoint_by_environment = {
    preprod = "network-api-internal.private-api.cloud-preprod.yandex.net"
    staging = "network-api.private-api.cloud.yandex.net"
    prod = "network-api.private-api.cloud.yandex.net"
  }

  vpc_endpoint_by_environment = {
    preprod = "api-adapter.private-api.ycp.cloud-preprod.yandex.net"
    staging = "api-adapter.private-api.ycp.cloud.yandex.net"
    prod = "api-adapter.private-api.ycp.cloud.yandex.net"
  }

  service_config = {
    # zkConfig = {
    #   endpoints = module.constants.by_environment.zk_endpoints
    #   root      = "/node_deployer"
    # }

    grpcServer = {
      maxConcurrentRequests = 10
      port                  = 443
    }

    # authorizationConfig = {
    #   enabled              = true
    #   folderId             = module.constants.by_environment.node_deployer_access_folder
    #   isFeatureFlagEnabled = module.constants.by_environment.node_deployer_feature_flag_enabled
    #   accessServiceEndpoint = module.constants.by_environment.access_service_endpoint 
    # }

    # resourcePoolConfig = {
    #   servantSiteFolderId = module.constants.by_environment.node_deployer_access_folder
    #   creationFolder      = module.constants.by_environment.node_deployer_creation_folder
    #   host                = module.constants.by_environment.resource_pool_host
    #   port                = 9090
    #   timeout             = 60
    # }

    # s3Config = {
    #   // FIXME: Pass this variables via SKM
    #   //        node_deployer_service code modification is required
    #   accessKey = var.s3_access_key
    #   secretKey = var.s3_secret_key
    #   endpoint  = local.s3_endpoint_by_environment[var.environment]
    #   region    = "us-east-1"
    # }

    dbConfig = {
      url      = local.dburl_by_environment[var.environment]
      username = "datasphere"
      password = var.db_password
    }

    unifiedAgentConfiguration = {
      address = "127.0.0.1"
      port    = 16399
    }

    authClientConfig = {
      host   = "as.private-api.cloud.yandex.net"
      port   = 4286
      maxRetries = 10
      timeout = "PT0.2S"
    }

    billingSpecs = {
      stt_streaming_units_v1 = {
        schema = "ai.speech.stt.hybrid.v1"
        units  = "15sec"
      }
      tts_units = {
        schema = "ai.speech.tts.hybrid.v1"
        units  = "request"
      }
    }

    validityPeriod = 604800

    # sgConfig = {
    #   host = local.sg_endpoint_by_environment[var.environment]
    #   port = 9823
    #   timeout = 30000
    # }

    # vpcConfig = {
    #   host = local.vpc_endpoint_by_environment[var.environment]
    #   port = 443
    #   timeout = 30000
    # }

    # nodeDefaultParameters = {
    #   subnets = module.well_known_networks.user_nets.subnets
    # }
  }
}

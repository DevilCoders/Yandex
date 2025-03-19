module "well_known_networks" {
  source                   = "../../../../data_sources/networks"
  environment              = var.environment
  service_account_key_file = var.service_account_key_file
}

module "well_known_dns_zones" {
  source                   = "../../../../data_sources/dns_zones"
  environment              = var.environment
  service_account_key_file = var.service_account_key_file
}

locals {
  authorization_endpoint_by_environment = {
    preprod = "as.private-api.cloud-preprod.yandex.net"
    staging = "as.private-api.cloud.yandex.net"
    prod    = "as.private-api.cloud.yandex.net"
  }

  service_config = {
    zkConfig = {
      endpoints = module.constants.by_environment.zk_endpoints
      root      = "/node_deployer"
    }
    grpcServer = {
      maxConcurrentRequests = 100
      port                  = 443
    }
    servantClientConfig = {
      port              = 9092
      connectionTimeout = 10
    }
    authorizationEnabled = true
    accessServiceHostname = local.authorization_endpoint_by_environment[var.environment]
  }
}

module "service_container" {
  source = "../../../../modules/container"

  environment = var.environment
  name        = "node-service"

  tag = "0.19"
  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/node-service"
    staging = "cr.yandex/crppns4pq490jrka0sth/node-service"
    prod    = "cr.yandex/crppns4pq490jrka0sth/node-service"
  }

  ports = [
    443
  ]

  mounts = {
    yandex-configs = {
      path      = "/etc/yandex"
      read_only = true
    }
    etc-secrets = {
      path      = "/etc/secrets"
      read_only = true
    }
    var-log = {
      path      = "/var/log"
      read_only = false
    }
  }

  envvar = [
    "APPLICATION_YAML",
    "ZOOKEEPER_SERVERS",
    "ENVIRONMENT_TYPE"
  ]
}

locals {
  service_component = {
    container = module.service_container.container
    configs = {
      "/etc/yandex/ai/node-service.yaml" = yamlencode(local.service_config)
    }
    envvar = {
      APPLICATION_YAML  = "/etc/yandex/ai/node-service.yaml"
      ZOOKEEPER_SERVERS = module.constants.by_environment.zk_servers
    }
    volumes = {}
  }

  target_group_name_by_environment = {
    preprod = ["ai-node-service-preprod", null]
    staging = ["ai-node-service-preprod", null]
    prod    = ["ai-node-service-prod", null]
  }
}

module "service" {
  source = "../../../../modules/standard_service"

  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  environment        = var.environment
  folder_id          = var.folder_id
  service_account_id = var.service_account_id

  name        = "node-service"
  description = "node-service service" # data.template_file.ig_description.rendered

  components = [
    local.service_component,
    module.solomon_agent_component,
    module.logrotate_component,
    module.push_client_component,
    module.cadvisor_component,
    module.configs_deployer_component
  ]

  networks = [
    {
      interface = module.well_known_networks.control_nets_yandex_ru
      dns = [{
        dns_zone_id = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
        fqdn        = "node-service-${var.environment}-{instance.index}.${module.well_known_dns_zones.ds_private_dns_zone.zone}"
      }]
    },
    {
      interface = module.well_known_networks.control_nets_ds_ipv6
      dns       = []
    }
  ]

  zones = [
    "ru-central1-a"
  ]

  instance_group_size = var.instance_group_size

  resources_per_instance = {
    cores  = 4
    memory = 16
    gpus   = 0
  }

  instance_disk_desc = {
    size = "32"
    type = "network-ssd"
  }

  image_id = module.constants.by_environment.kubelet_cpu_image

  deploy_policy = {
    max_unavailable  = 1
    max_creating     = 3
    max_expansion    = 0
    max_deleting     = 1
    startup_duration = "300s"
  }

  skm_bundle = module.skm_bundle.bundle

  target_group_name        = local.target_group_name_by_environment[var.environment][0]
  target_group_description = local.target_group_name_by_environment[var.environment][1]

  abc_service = "yc_datasphere"
}

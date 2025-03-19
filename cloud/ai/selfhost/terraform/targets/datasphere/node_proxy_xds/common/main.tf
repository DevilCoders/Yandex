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

module "service_container" {
  source = "../../../../modules/container"

  environment = var.environment
  name        = "node-proxy-xds"

  tag = "0.68"
  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/hybrid-balancer/xds-service"
    staging = "cr.yandex/crppns4pq490jrka0sth/hybrid-balancer/xds-service"
    prod    = "cr.yandex/crppns4pq490jrka0sth/hybrid-balancer/xds-service"
  }

  ports = [
    5678
  ]

  mounts = {
    var-log-envoy-xds = {
      path      = "/var/log/envoy-xds"
      read_only = true
    }
  }

  envvar = [
    #"NODE_CONFIG_PATH",
    "ZK_CONNECTION_STRING",
    "ENVIRONMENT_TYPE",
    "INSTANCE_ZONE_ID"
  ]
}

locals {
  service_component = {
    container = module.service_container.container
    configs   = {}
    envvar    = {}
    volumes   = {}
  }

  target_group_name_by_environment = {
    preprod = ["node-proxy-xds-preprod", "node-proxy-xds-preprod"]
    staging = ["ai-node-proxy-xds-preprod", null]
    prod    = ["ai-node-proxy-xds-prod", null]
  }

  zones = keys(module.well_known_networks.control_nets_ds_ipv6.subnets)
}

module "service" {
  source = "../../../../modules/standard_service"

  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  environment        = var.environment
  folder_id          = var.folder_id
  service_account_id = var.service_account_id

  name        = "node-proxy-xds"         #"${var.name}-${var.environment}"
  description = "node-proxy-xds service" # data.template_file.ig_description.rendered

  components = [
    local.service_component,
    module.unified_agent_component,
    module.logrotate_component,
    module.push_client_component,
    module.cadvisor_component,
    module.configs_deployer_component
  ]

  // TODO: Fix this inconsistent placement
  networks = var.environment == "preprod" ? [
    {
      interface = module.well_known_networks.control_nets_yandex_ru
      dns = [{
        dns_zone_id = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
        fqdn        = "node-proxy-xds-${var.environment}-{instance.index}.${module.well_known_dns_zones.ds_private_dns_zone.zone}"
      }]
    },
    {
      interface = module.well_known_networks.control_nets_ds_ipv6
      dns       = []
    }
    ] : [
    {
      interface = module.well_known_networks.cloud_ml_dev_nets
      dns = [{
        dns_zone_id = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
        fqdn        = "node-proxy-xds-${var.environment}-{instance.index}.${module.well_known_dns_zones.ds_private_dns_zone.zone}"
      }]
    },
    {
      interface = module.well_known_networks.control_nets_ds_ipv6
      dns       = []
    }
  ]

  zones = local.zones

  instance_group_size = length(local.zones)

  resources_per_instance = {
    cores  = 4
    memory = 8
    gpus   = 0
  }

  instance_disk_desc = {
    size = "32"
    type = "network-ssd"
  }

  secondary_disk_specs = []

  image_id = module.constants.by_environment.kubelet_cpu_image

  deploy_policy = {
    max_unavailable  = 1
    max_creating     = 3
    max_expansion    = 0
    max_deleting     = 1
    startup_duration = "300s"
  }

  additional_volumes = {
    var-log-envoy-xds = {
      hostPath = {
        path = "/var/log/envoy-xds"
      }
    }
  }

  additional_envvar = {
    ZK_CONNECTION_STRING = module.constants.by_environment.zk_servers
    ENVIRONMENT_TYPE     = var.environment
    INSTANCE_ZONE_ID     = "{instance.zone_id}"
  }

  additional_runcmd = []

  additional_userdata = {
    mounts = []
  }

  skm_bundle = module.skm_bundle.bundle

  target_group_name        = local.target_group_name_by_environment[var.environment][0]
  target_group_description = local.target_group_name_by_environment[var.environment][1]

  abc_service = "yc_datasphere"
}

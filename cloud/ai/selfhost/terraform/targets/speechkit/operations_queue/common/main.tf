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

  name = "operations-queue"

  tag = "0.243"

  repositories = {
    staging = "registry.yandex.net/cloud-ai/operations-queue"
    prod    = "registry.yandex.net/cloud-ai/operations-queue"
  }

  ports = [
    443
  ]

  mounts = {
    var-log-yc-ai = {
      path      = "/var/log"
      read_only = false
    }
    var-lib-billing = {
      path      = "/var/lib/billing"
      read_only = false
    }
    run-prometheus = {
      path      = "/run/prometheus"
      read_only = true
    }
    data-sa-keys = {
      path      = "/etc/yc"
      read_only = true
    }
  }

  envvar = [
    "ENVIRONMENT_TYPE",
    "ZOOKEEPER_SERVERS",
    "CLOUD_INSTALLATION"
  ]
}

locals {
  service_component = {
    container = module.service_container.container

    configs = {}

    envvar = {
      CLOUD_INSTALLATION = "1"
      ZOOKEEPER_SERVERS = module.constants.by_environment.zk_servers
    }

    volumes = {}
  }

  /*
   * TODO: This is hack to prevent changes in the existing platform_l7_load_balancer_spec
   *       changes in this cause instance group to deny appplication of changes
   *       onto existing groups
   *       Also platform_l7_load_balancer_spec is deprecated field and should be replaced
   *       with applicaitonb_load_balander_spec for new instance group
   *       Currenlty there is only one way to change this via recreation of instance group
   *       We should align this names among all igs and use alb specs instead
   */
  target_group_name_by_environment = {
    staging = ["ai-operations-queue-preprod", null]
    prod    = ["ai-operations-queue-prod", null]
  }
}

module "service" {
  source = "../../../../modules/standard_service"

  # Secrets passthrough
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  # Target passthrough
  environment        = var.environment
  folder_id          = var.folder_id
  service_account_id = var.service_account_id

  # Service description
  name        = "speechkit-operations-queue"
  description = "SpeechKit Operations Queue"

  components = [
    local.service_component,
    module.solomon_agent_component,
    module.logrotate_component,
    module.push_client_component,
    module.push_client_billing_component,
    module.cadvisor_component,
    module.configs_deployer_component
  ]

  // Resources
  networks = [
    {
      interface = module.well_known_networks.cloud_ml_dev_nets
      dns = [{
        dns_zone_id = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
        fqdn        = "speechkit-operations-queue-${var.environment}-{instance.index}.${module.well_known_dns_zones.ds_private_dns_zone.zone}"
      }]
    }
  ]

  zones = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c"
  ]

  instance_group_size = var.instance_group_size

  resources_per_instance = {
    cores  = 16
    memory = 64
    gpus   = 0
  }

  instance_disk_desc = {
    size = "256"
    type = "network-ssd"
  }

  deploy_policy = {
    max_unavailable  = 1
    max_creating     = 30
    max_expansion    = 0
    max_deleting     = 4
    startup_duration = "120s" # var.startup_duration
  }

  skm_bundle = module.skm_bundle.bundle

  target_group_name        = local.target_group_name_by_environment[var.environment][0]
  target_group_description = local.target_group_name_by_environment[var.environment][1]
}

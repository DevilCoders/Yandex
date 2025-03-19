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

  name = "ds-billing-service"

  tag = "0.10"

  /*
   * TODO: Currently we are able to use registry from cloud prod
   *       in preprod environment because we passing the same docker
   *       credentials in both environment
   *       Decide: do we need separate registries for each env?
   */
  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/ds-billing-service"
    staging = "cr.yandex/crppns4pq490jrka0sth/ds-billing-service"
    prod    = "cr.yandex/crppns4pq490jrka0sth/ds-billing-service"
  }

  ports = [
    443
  ]

  mounts = {
    yandex-configs = {
      path      = "/etc/yandex"
      read_only = false
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
    "ENVIRONMENT_TYPE",
    "ZOOKEEPER_SERVERS",
    "APPLICATION_YAML"
  ]
}

locals {
  service_component = {
    container = module.service_container.container

    configs = {
      "/etc/yandex/ai/ds-billing-service.yaml" = yamlencode(local.service_config)
    }

    envvar = {
      //ENVIRONMENT_TYPE = var.environment
      ZOOKEEPER_SERVERS = module.constants.by_environment.zk_servers
      APPLICATION_YAML  = "/etc/yandex/ai/ds-billing-service.yaml"
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
    // preprod = ["node-deployer-service-preprod", "node-deployer-service-preprod"]
    staging = ["ds-billing-service-staging", null]
    prod    = ["ds-billing-service-prod", null]
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
  name        = "ds-billing-service"
  description = "DS Billing Service"

  components = [
    local.service_component,
    module.solomon_agent_component,
    module.logrotate_component,
    module.push_client_component,
    module.cadvisor_component,
    module.configs_deployer_component,
    module.unified_agent_component
  ]
  // Resources
  /*
   * TODO: Currently there is inconsistency between preproduciton and production clouds
   *       need to align networks that are used between all of environments
   *       and remove condition on environment
   */
  networks = [
    {
      interface = module.well_known_networks.control_nets_yandex_ru
      dns = [{
        dns_zone_id = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
        fqdn        = "ds-billing-service-${var.environment}-{instance.index}.${module.well_known_dns_zones.ds_private_dns_zone.zone}"
      }]
    }
  ]

  zones               = ["ru-central1-a"]
  instance_group_size = 1

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

  deploy_policy = {
    max_unavailable  = 1
    max_creating     = 1
    max_expansion    = 0
    max_deleting     = 1
    startup_duration = "60s" # var.startup_duration
  }

  skm_bundle = module.skm_bundle.bundle

  target_group_name        = local.target_group_name_by_environment[var.environment][0]
  target_group_description = local.target_group_name_by_environment[var.environment][1]

  abc_service = "yc_datasphere"
}

resource "yandex_alb_backend_group" "ds-billing-service-backend-group" {
  folder_id          = var.folder_id

  grpc_backend {
    name = "main"
    port = 443
    target_group_ids = [
      module.service.instance_group.application_load_balancer_state[0].target_group_id
    ]
  }
}

resource "yandex_alb_http_router" "ds-billing-service-router" {
  folder_id          = var.folder_id
}

resource "yandex_alb_virtual_host" "ds-billing-service-virtual-host" {
  #folder_id          = var.folder_id
  name = "ds-billing-service-virtual-host"
  http_router_id = yandex_alb_http_router.ds-billing-service-router.id
  route {
    name = "main-route"
    grpc_route {
      grpc_route_action {
        backend_group_id = yandex_alb_backend_group.ds-billing-service-backend-group.id
        max_timeout = "3s"
      }
    }
  }
}

resource "yandex_alb_load_balancer" "ds-billing-service-balancer" {
  folder_id          = var.folder_id

  network_id  = module.well_known_networks.control_nets_yandex_ru.network
  allocation_policy {
    location {
      zone_id   = "ru-central1-a"
      subnet_id = module.well_known_networks.control_nets_yandex_ru.subnets["ru-central1-a"]
    }
  }

  listener {
    name = "my-listener"
    endpoint {
      address {
        external_ipv4_address {
        }
      }
      ports = [ 443 ]
    }    
    tls {
      default_handler {
          certificate_ids = [ var.tls_certificate_id ]
          http_handler {
              http_router_id = yandex_alb_http_router.ds-billing-service-router.id
            }
        }
    }
  }    
}

locals {
  dns_record_zone_id_by_environment = {
    preprod = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
    staging = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
    prod    = module.well_known_dns_zones.ds_public_dns_zone != null ? module.well_known_dns_zones.ds_public_dns_zone.dns_zone_id : null
  }

  dns_record_name_by_environment = {
    preprod = "billing"
    staging = "billing"
    prod    = "billing"
  }
}

resource "yandex_dns_recordset" "ds-billing-service-dns-recordset" {
  zone_id = local.dns_record_zone_id_by_environment[var.environment]
  name    = local.dns_record_name_by_environment[var.environment]
  type    = "A"
  ttl     = 200
  data    = [
    for listener in resource.yandex_alb_load_balancer.ds-billing-service-balancer.listener:
      listener.endpoint[0].address[0].external_ipv4_address[0].address
  ]
}
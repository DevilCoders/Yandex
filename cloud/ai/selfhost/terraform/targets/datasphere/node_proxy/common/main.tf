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

  name = "node-proxy"

  tag = "0.1"

  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/envoyproxy/envoy-dev"
    staging = "cr.yandex/crppns4pq490jrka0sth/envoyproxy/envoy-dev"
    prod    = "cr.yandex/crppns4pq490jrka0sth/envoyproxy/envoy-dev"
  }

  ports = [
    8080
  ]

  mounts = {
    envoy-config = {
      path      = "/etc/envoy"
      read_only = true
    }
    var-log-envoy = {
      path      = "/var/log/envoy"
      read_only = false
    }
    envoy-certs = {
      path      = "/etc/certs"
      read_only = true
    }
  }

  envvar = ["INSTANCE_ZONE_ID"]

  /*
   * TODO: Find a better way to specify zone specific config
   *       currently we relying
   */
  command = [
    "/bin/bash"
  ]
  args = [
    "-c",
    "/usr/local/bin/envoy -c /etc/envoy/envoy_$INSTANCE_ZONE_ID.yaml -l info 2>&1 | tee /var/log/envoy/process.log"
  ]
}

module "auth_container" {
  source = "../../../../modules/container"

  environment = var.environment

  name = "node-proxy-iam"

  # tag = "0.240"
  # repositories = {
  #   preprod = "registry.yandex.net/cloud-ai/envoy-iam"
  #   staging = "registry.yandex.net/cloud-ai/envoy-iam"
  #   prod    = "registry.yandex.net/cloud-ai/envoy-iam"
  # }
  # command = [
  #   "/root/run.sh"
  # ]

  tag = "0.19"

  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/envoyproxy/access-service"
    staging = "cr.yandex/crppns4pq490jrka0sth/envoyproxy/access-service"
    prod    = "cr.yandex/crppns4pq490jrka0sth/envoyproxy/access-service"
  }

  # args = [
  #   "-port",
  #   "40002"
  # ]

  ports = [
    40002
  ]

  # TODO: Why this override is required?
  envvar = [
    "ENVIRONMENT_TYPE",
    "MICRONAUT_ENVIRONMENTS"
  ]
}

module "billing_container" {
  source = "../../../../modules/container"

  environment = var.environment

  name = "node-proxy-billing"

  tag = "0.8"

  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/envoyproxy/billing-service"
    staging = "cr.yandex/crppns4pq490jrka0sth/envoyproxy/billing-service"
    prod    = "cr.yandex/crppns4pq490jrka0sth/envoyproxy/billing-service"
  }

  // TODO: Add this fields
  # command = [
  #   "/root/run.sh"
  # ]

  # args = [
  #   "-port",
  #   "40002"
  # ]

  ports = [
    40003
  ]

  # TODO: Why this override is required?
  envvar = [
    "ENVIRONMENT_TYPE",
    "MICRONAUT_ENVIRONMENTS"
  ]
}

module "healthcheck_container" {
  source = "../../../../modules/container"

  environment = var.environment

  name = "nginx"

  tag = "latest"
  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/nginx"
    staging = "cr.yandex/crppns4pq490jrka0sth/nginx"
    prod    = "cr.yandex/crppns4pq490jrka0sth/nginx"
  }

  ports = [
    80
  ]
}

locals {
  service_component = {
    container = module.service_container.container
    configs = {
      for zone, config in local.config :
      "/etc/envoy/envoy_${zone}.yaml" => yamlencode(config)
    }
    envvar = {}
    volumes = {
      envoy-certs = {
        hostPath = {
          path = "/etc/certs"
        }
      }
    }
  }

  auth_component = {
    container = module.auth_container.container
    configs   = {}
    envvar    = {}
    volumes   = {}
  }

  billing_component = {
    container = module.billing_container.container
    configs   = {}
    envvar = {
      MICRONAUT_ENVIRONMENTS = var.environment
    }
    volumes = {}
  }

  healthcheck_component = {
    container = module.healthcheck_container.container
    configs   = {}
    envvar    = {}
    volumes   = {}
  }

  target_group_name_by_environment = {
    preprod = ["node-proxy-preprod", "node-proxy-preprod"]
    staging = ["ai-node-proxy-preprod", null]
    prod    = ["ai-node-proxy-prod", null]
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
  name        = "node-proxy"                          # "${var.name}-${var.environment}"
  description = "node-proxy service powered by Envoy" # data.template_file.ig_description.rendered

  components = [
    local.service_component,
    local.auth_component,
    local.billing_component,
    local.healthcheck_component,
    module.logrotate_component,
    module.push_client_component,
    module.cadvisor_component,
    module.configs_deployer_component,
    module.unified_agent_component
  ]

  // Resources
  // TODO: Fix this inconsistent placement
  networks = var.environment == "preprod" ? [
    {
      interface = module.well_known_networks.control_nets_yandex_ru,
      dns = [{
        dns_zone_id = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
        fqdn        = "node-proxy-${var.environment}-{instance.index}.${module.well_known_dns_zones.ds_private_dns_zone.zone}"
      }]
    },
    {
      // TODO: ??? Why are control nets required?
      interface = module.well_known_networks.control_nets_ds_ipv6
      dns       = []
    }
    ] : [
    {
      interface = module.well_known_networks.cloud_ml_dev_nets,
      dns = [{
        dns_zone_id = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
        fqdn        = "node-proxy-${var.environment}-{instance.index}.${module.well_known_dns_zones.ds_private_dns_zone.zone}"
      }]
    },
    {
      // TODO: ??? Why are control nets required?
      interface = module.well_known_networks.control_nets_ds_ipv6
      dns       = []
    }
  ]

  zones               = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
  instance_group_size = 3

  resources_per_instance = {
    cores  = 8
    memory = 16
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
    startup_duration = "60s"
  }

  // Custom parameters
  additional_volumes = {
    envoy-config = {
      hostPath = {
        path = "/etc/envoy"
      }
    }
    var-log-envoy = {
      hostPath = {
        path = "/var/log/envoy"
      }
    }
  }

  additional_envvar = {
    INSTANCE_ZONE_ID = "{instance.zone_id}"
  }

  skm_bundle = module.skm_bundle.bundle

  nlb_target = true

  target_group_name        = local.target_group_name_by_environment[var.environment][0]
  target_group_description = local.target_group_name_by_environment[var.environment][1]

  abc_service = "yc_datasphere"

  # additional_userdata = {
  #   enable-oslogin = true
  # }
}

data "yandex_compute_instance_group" "ig_public_datasource" {
  instance_group_id = module.service.instance_group.id
}

resource "yandex_lb_target_group" "node_proxy_nlb_target_group" {
  name      = "node-proxy-target-group"
  region_id = "ru-central1"

  dynamic "target" {
    for_each = data.yandex_compute_instance_group.ig_public_datasource.instances

    # FIXME: This is HUGE assumption that network_interface with id == 0
    #        if correct one to provide to NLB
    #        need to find a better way to handling it
    content {
      subnet_id = target.value.network_interface[0].subnet_id
      address   = target.value.network_interface[0].ipv6_address
    }
  }

  dynamic "target" {
    for_each = data.yandex_compute_instance_group.ig_public_datasource.instances

    # FIXME: This is HUGE assumption that network_interface with id == 0
    #        if correct one to provide to NLB
    #        need to find a better way to handling it
    content {
      subnet_id = target.value.network_interface[0].subnet_id
      address   = target.value.network_interface[0].ip_address
    }
  }
}

# resource "yandex_lb_network_load_balancer" "node_proxy_nlb_ipv6" {
#   name = "node-proxy-load-balancer"

#   listener {
#     name = "node-proxy-nlb-listener-ipv6"
#     port = 443
#     target_port = 8080
#     external_address_spec {
#       ip_version = "ipv6"
#     }
#   }


#   attached_target_group {
#     target_group_id = resource.yandex_lb_target_group.node_proxy_nlb_target_group.id

#     healthcheck {
#       name = "tcp"
#       tcp_options {
#         port = 80
#       }
#     }
#   }
# }

resource "yandex_lb_network_load_balancer" "node_proxy_nlb_ipv4" {
  name = "node-proxy-load-balancer"

  listener {
    name = "node-proxy-nlb-listener-ipv4"
    port = 443
    target_port = 8080
    external_address_spec {
      ip_version = "ipv4"
    }
  }

  attached_target_group {
    target_group_id = resource.yandex_lb_target_group.node_proxy_nlb_target_group.id

    healthcheck {
      name = "tcp"
      tcp_options {
        port = 80
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
    preprod = "node-api"
    staging = "node-api-staging"
    prod    = "node-api"
  }
}

# resource "yandex_dns_recordset" "node_api_datasphere_cloud_yandex_net_aaaa" {
#   zone_id = local.dns_record_zone_id_by_environment[var.environment]
#   name    = local.dns_record_name_by_environment[var.environment]
#   type    = "AAAA"
#   ttl     = 200
#   data    = [
#     for listener in resource.yandex_lb_network_load_balancer.node_proxy_nlb_ipv6.listener:
#       listener.external_address_spec.*.address[0]
#   ]
# }

resource "yandex_dns_recordset" "node_api_datasphere_cloud_yandex_net_a" {
  zone_id = local.dns_record_zone_id_by_environment[var.environment]
  name    = local.dns_record_name_by_environment[var.environment]
  type    = "A"
  ttl     = 200
  data    = [
    for listener in resource.yandex_lb_network_load_balancer.node_proxy_nlb_ipv4.listener:
      listener.external_address_spec.*.address[0]
  ]
}

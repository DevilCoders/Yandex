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

  name = "zookeeper"

  tag = "latest"

  /*
   * TODO: Currently we are able to use registry from cloud prod
   *       in preprod environment because we passing the same docker
   *       credentials in both environment
   *       Decide: do we need separate registries for each env?
   */
  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/zk"
    staging = "cr.yandex/crppns4pq490jrka0sth/zk"
    prod    = "cr.yandex/crppns4pq490jrka0sth/zk"
  }

  ports = [
    2181,
    2888,
    3888,
    17006
  ]

  mounts = {
    zk-dir = {
      path      = "/zk"
      read_only = false
    }
    all-configs = {
      path      = "/etc"
      read_only = false
    }
  }
  envvar = [
    "ZOOCFGDIR",
    "ZOO_CONF_DIR",
    "ZOO_DATA_DIR",
    "ZOO_DATA_LOG_DIR",
    "JVMFLAGS"
  ]
}

locals {
  service_component = {
    container = module.service_container.container

    configs = {
      "/etc/zk_conf/zoo.cfg"           = data.external.zoo_cfg_getter.result.rendered
      "/etc/zk_conf/configuration.xsl" = file("${path.module}/files/configuration.xsl")
      "/etc/zk_conf/log4j.properties"  = file("${path.module}/files/log4j.properties")
    }

    envvar = {
      ZOOCFGDIR        = "/etc/zk_conf"
      ZOO_CONF_DIR     = "/etc/zk_conf"
      ZOO_DATA_DIR     = "/zk/data"
      ZOO_DATA_LOG_DIR = "/zk/datalog"
      JVMFLAGS         = "-Djava.net.preferIPv6Addresses=true"
    }

    volumes = {
      zk-dir = {
        hostPath = {
          path = "/mnt/zk-dir"
        }
      }
    }
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
    preprod = ["zookeeper-preprod", "zookeeper-preprod"]
    staging = ["ai-zk-service-preprod", null]
    prod    = ["ai-zk-service-prod", null]
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
  name        = "zookeeper"                    # "${var.name}-${var.environment}"
  description = "YC AI Zookeeper installation" # data.template_file.ig_description.rendered

  components = [
    local.service_component,
    module.solomon_agent_component,
    module.logrotate_component,
    module.push_client_component,
    module.cadvisor_component,
    module.configs_deployer_component
  ]

  // Resources
  /*
   * TODO: Currently there is inconsistency between preproduciton and production clouds
   *       need to align networks that are used between all of environments
   *       and remove condition on environment
   */
  networks = var.environment == "preprod" ? [
    {
      interface = module.well_known_networks.control_nets_yandex_ru
      dns = [{
        dns_zone_id = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
        fqdn        = "zookeeper-${var.environment}-{instance.index}.${module.well_known_dns_zones.ds_private_dns_zone.zone}"
      }]
    }
    ] : [
    {
      interface = module.well_known_networks.cloud_ml_dev_nets
      dns = [{
        dns_zone_id = module.well_known_dns_zones.ds_private_dns_zone.dns_zone_id
        fqdn        = "zookeeper-${var.environment}-{instance.index}.${module.well_known_dns_zones.ds_private_dns_zone.zone}"
      }]
    }
  ]

  zones = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c"
  ]

  instance_group_size = 3

  resources_per_instance = {
    cores  = 2
    memory = 8
    gpus   = 0
  }

  instance_disk_desc = {
    size = "32"
    type = "network-ssd"
  }

  // Disk to persist ZK state
  secondary_disk_specs = [
    {
      device_name = "data"
      mode : "READ_WRITE"
      disk_spec = {
        type_id = "network-ssd"
        size    = 10
      }
    }
  ]

  deploy_policy = {
    // NOTE: If more than (instance_group_size / 2) will be unavailable
    //       quorum will be lost during the deployment
    max_unavailable  = 1
    max_creating     = 3
    max_expansion    = 0
    max_deleting     = 1
    startup_duration = "180s"
  }

  // Custom parameters
  additional_runcmd = [
    ["sudo", "bash", "-c", "mkdir -p /mnt/zk-dir/data; mkdir -p /mnt/zk-dir/datalog;  curl -H 'Metadata-Flavor: Google' http://169.254.169.254/computeMetadata/v1/instance/attributes/instance_index > /mnt/zk-dir/data/myid"],
    ["sudo", "bash", "-c", "adduser --disabled-password --gecos \"\" zookeeper"]
  ]

  /*
   * TODO: This data should be part of secondary_disk_spec because it contains
   *       information about how to attach provided network disk
   */
  additional_userdata = {
    disk_setup = {
      "/dev/disk/by-id/virtio-data" = {
        table_type = "mbr"
        overwrite  = false
      }
    }
    fs_setup = [{
      filesystem = "ext4"
      device     = "/dev/disk/by-id/virtio-data"
      overwrite  = false
    }]
    mounts = [["/dev/disk/by-id/virtio-data", "/mnt/zk-dir", "auto", "defaults", "0", "0"]]
  }

  skm_bundle = module.skm_bundle.bundle

  target_group_name        = local.target_group_name_by_environment[var.environment][0]
  target_group_description = local.target_group_name_by_environment[var.environment][1]

  abc_service = "yc_datasphere"
}

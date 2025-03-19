locals {
  default_env_vars = [
    {
      name = "CLOUD_INSTALLATION"
      value: "1"
    },
    {
      name = "ENABLE_GPU_FORWARD"
      value: "1"
    },
    {
      name = "ENVIRONMENT_TYPE"
      value: var.environment
    },
    {
      name = "ZOOKEEPER_SERVERS"
      value: module.constants.by_environment.zk_servers
    }
  ]

  env_secrets = [
    {
      name = "REDIS_PASSWORD"
      path: module.skm_bundle.path.managed-redis-password
    },
    {
      name = "VISION_TEXT_DETECTION_API_KEY"
      path: module.skm_bundle.path.vision-textdetection-api-key
    }
  ]

  env_vars = {
    env = concat(local.default_env_vars, var.custom_env_vars)
  }
}

provider "yandex" {
  endpoint  = var.yc_endpoint
  folder_id = var.yc_folder
  zone      = var.yc_zone
}

module "constants" {
  source       = "../constants"
  environment = var.environment
}

module "push_client_config" {
  source        = "../../modules/push_client_config"
  files         = [
    {
      name       = "/var/log/yc/ai/services-proxy.log"
      send_delay = 30
      topic      = module.constants.by_environment.topic_logs_services_proxy
    },
  ]
}

module "push_client_billing_config" {
  source                = "../../modules/push_client_billing_config"
  topic_billing         = module.constants.by_environment.topic_billing
}

module "solomon_config" {
    source = "../../modules/solomon_config"
    shard_service = "vision"
    cluster = module.constants.by_environment.solomon_cluster
    yav_token = var.yandex_token
    yav_id = module.constants.by_environment.env_secret
    yav_value_name = "service-key-services-proxy-deployer"
    sa_public_key_path = "/etc/yc/public.pem"
    sa_private_key_path = "/etc/yc/private.pem"
}

data "template_file" "configs" {
  template = file("${path.module}/files/configs.tpl")

  vars = {
    push_client_conf              = module.push_client_config.rendered
    push_client_billing_conf      = module.push_client_billing_config.rendered
    solomon_agent_conf            = module.solomon_config.solomon-agent
    solomon_agent_extra_conf      = module.solomon_config.solomon-agent-extra
    logrotate_app_conf            = file("${path.module}/files/logrotate-app.conf")
  }
}

data "template_file" "ig_description" {
  template = file("${path.module}/files/description.tpl")

  vars = {
    app_image   = var.app_image
    app_version = var.app_version
    environment = var.environment
  }
}

module "docker_config" {
  source       = "../../modules/docker_config"
  yandex_token = var.yandex_token
  secret       = module.constants.by_environment.common_secret
}

module "user_data" {
  source       = "../../modules/user_data"
  yandex_token = var.yandex_token
  extra_bootcmd = [
    [ "bash", "-c", "/usr/bin/skm decrypt"]
  ]
  extra_runcmd = [
    [ "bash", "-c", "/usr/bin/skm decrypt"]
  ]
}

module "ai_services_proxy_instance_group_common" {
  // TODO: Fix that hack. Currently instance_group resource cannot work properly
  //       because it is not possible to specify platform balancer
  // source              = "../../modules/instance_group"
  source              = "../../modules/instance_group_ycp"

  // General
  folder_id           = var.yc_folder
  service_account_id  = var.yc_sa_id

  // Instance group parameters
  name                = "${var.name}-${var.environment}"
  description         = data.template_file.ig_description.rendered
  target_group_name   = "${var.name}-${var.environment}"
  instance_group_size = var.yc_instance_group_size
  zones               = var.yc_zones

  // Deploy policy
  max_unavailable     = var.max_unavailable
  max_creating        = var.max_creating
  max_expansion       = var.max_expansion
  max_deleting        = var.max_deleting
  startup_duration    = var.startup_duration

  // Instance config
  platform_id         = var.platform_id
  cores_per_instance  = var.instance_cores
  memory_per_instance = var.instance_memory
  disk_per_instance   = var.instance_disk_size
  disk_type           = var.instance_disk_type
  subnets             = module.constants.by_environment.subnets
  image_id            = var.image_id

  // Instance SW config
  configs              = data.template_file.configs.rendered
  podmanifest          = templatefile("${path.module}/files/podmanifest.tpl.yml", {
      config_digest = sha256(data.template_file.configs.rendered)
      metadata_version = var.metadata_version
      solomon_version = var.solomon_agent_version
      push_client_version = var.push_client_version
      juggler_version = var.juggler_version
      logrotate_version = var.logrotate_version
      app_image = var.app_image
      app_version = var.app_version
      pdf_converter_version = var.pdf_converter_version
      nvidia_dcgm_exporter_version = var.nvidia_dcgm_exporter_version
      cadvisor_version = var.cadvisor_version
      env_vars = indent(6, yamlencode(local.env_vars))
      env_secrets = local.env_secrets
  })
  docker_config        = module.docker_config.rendered
  skip_update_ssh_keys = "true"

  metadata = {
    user-data = module.user_data.rendered
    skm = module.skm_bundle.bundle
    nsdomain = "{instance.internal_dc}.ycp.cloud.yandex.net"
    shortname = "{instance_group.id}-{instance.short_id}"
  }

  labels = {
    layer   = "saas"
    abc_svc = "ycai"
    env     = var.environment
    conductor-group = "yc_ai_prod"
    yandex-dns = "ig"
  }
}

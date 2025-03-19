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
      topic      = module.constants.by_environment.topic_logs_operations_queue
    },
  ]
}

module "push_client_billing_config" {
  source                = "../../modules/push_client_billing_config"
  topic_billing         = module.constants.by_environment.topic_billing
}

data "template_file" "configs" {
  template = file("${path.module}/files/configs.tpl")

  vars = {
    push_client_conf              = module.push_client_config.rendered
    push_client_billing_conf      = module.push_client_billing_config.rendered
    solomon_agent_conf            = file("${path.module}/files/solomon-agent.conf")
    solomon_agent_extra_conf      = file("${path.module}/files/solomon-agent-extra.conf")
    logrotate_app_conf            = file("${path.module}/files/logrotate-app.conf")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yml")

  vars = {
    config_digest                  = sha256(data.template_file.configs.rendered)
    metadata_version               = var.metadata_version
    solomon_version                = var.solomon_agent_version
    push_client_version            = var.push_client_version
    juggler_version                = var.juggler_version
    logrotate_version              = var.logrotate_version
    operations_queue_version       = var.operations_queue_version
    nvidia_dcgm_exporter_version   = var.nvidia_dcgm_exporter_version
    cadvisor_version               = var.cadvisor_version
    environment                    = var.environment == "preprod" ? "staging" : var.environment
    zk_servers                     = join(",", [for s in module.constants.by_environment.zk_global_endpoints : format("%s:%s", s.host, s.port)])
    instance_max_memory            = var.instance_max_memory,
    log_level = "DEBUG"
  }
}

data "template_file" "ig_description" {
  template = file("${path.module}/files/description.tpl")

  vars = {
    operations_queue_version       = var.operations_queue_version
    environment                    = var.environment
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

module "ai_operations_queue_instance_group_common" {
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

  // Instance HW config
  platform_id         = "standard-v2"
  cores_per_instance  = var.instance_cores
  memory_per_instance = var.instance_memory
  disk_per_instance   = var.instance_disk_size
  disk_type           = var.instance_disk_type
  subnets             = module.constants.by_environment.subnets
  image_id            = var.image_id

  // Instance SW config
  configs              = data.template_file.configs.rendered
  podmanifest          = data.template_file.podmanifest.rendered
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


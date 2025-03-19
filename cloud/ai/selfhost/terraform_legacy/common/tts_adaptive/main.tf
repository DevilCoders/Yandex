module "constants" {
  source      = "../constants"
  environment = var.environment
}

provider "yandex" {
  endpoint  = module.constants.by_environment.yc_endpoint
  folder_id = var.yc_folder
  zone      = var.yc_zone
}

provider "ycp" {
    prod = true
}

data "template_file" "configs" {
  template = file("${path.module}/files/configs.tpl")

  vars = {
    solomon_agent_conf            = file("${path.module}/files/solomon-agent.conf")
    solomon_agent_extra_conf = file("${path.module}/files/solomon-agent-extra.conf")
    logrotate_app_conf            = file("${path.module}/files/logrotate-app.conf")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yml")

  vars = {
    config_digest                  = sha256(data.template_file.configs.rendered)
    metadata_version               = var.metadata_version
    solomon_version                = var.solomon_agent_version
    logrotate_version              = var.logrotate_version
    tts_poc_version                = var.tts_poc_version
    cadvisor_version               = var.cadvisor_version
  }
}

data "template_file" "ig_description" {
  template = file("${path.module}/files/description.tpl")

  vars = {
    tts_poc_version             = var.tts_poc_version
    environment                 = var.environment
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
}

module "ai_instance_group_common" {
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
  platform_id         = "gpu-standard-v1"
  cores_per_instance  = var.instance_cores
  memory_per_instance = var.instance_memory
  gpus_per_instance   = var.instance_gpus
  disk_per_instance   = var.instance_disk_size
  disk_type           = var.instance_disk_type
  subnets             = module.constants.by_environment.subnets
  image_id            = var.image_id

  // Instance SW config
  configs              = data.template_file.configs.rendered
  podmanifest          = data.template_file.podmanifest.rendered
  docker_config        = module.docker_config.rendered
  skip_update_ssh_keys = "true"

  scheduling_policy = {
    termination_grace_period = "30s"
  }

  metadata = {
    user-data = module.user_data.rendered
  }

  labels = {
    layer   = "saas"
    abc_svc = "ycai"
    env     = var.environment
  }
}

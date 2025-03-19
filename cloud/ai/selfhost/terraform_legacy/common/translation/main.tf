provider "yandex" {
  endpoint                 = var.yc_endpoint
  folder_id                = var.yc_folder
  zone                     = var.yc_zone
}

module "constants" {
  source       = "../constants"
  environment = var.environment
}

module "push_client_config" {
  source        = "../../modules/push_client_config"
  tvm_client_id = module.constants.by_environment.tvm_client_id
  files         = [
    {
      name       = "/var/log/yc/ai/services-proxy.log"
      send_delay = 30
      topic      = module.constants.by_environment.topic_logs_services_proxy
    },
    {
      name       = "/var/log/translation-server/translate.log"
      topic      = module.constants.by_environment.topic_logs_common
      send_delay = 30
    },
    {
      name       = "/var/log/translation-server/nmt.log"
      topic      = module.constants.by_environment.topic_logs_common
      send_delay = 30
    },
    {
      name       = "/var/log/translation-server/error.log"
      topic      = module.constants.by_environment.topic_logs_common
      send_delay = 30
    }
  ]
}

module "push_client_billing_config" {
  source                = "../../modules/push_client_billing_config"
  tvm_client_billing_id = module.constants.by_environment.tvm_client_billing_id
  topic_billing         = module.constants.by_environment.topic_billing
}

data "template_file" "configs" {
  template = file("${path.module}/files/configs.tpl")

  vars = {
    push_client_conf                      = data.template_file.push_client_config.rendered
    push_client_billing_conf              = module.push_client_billing_config.rendered
    solomon_agent_conf                    = file("${path.module}/files/solomon-agent.conf")
    solomon_agent_translation_server_conf = file("${path.module}/files/solomon-agent-translation-server.conf")
    logrotate_app_conf                    = file("${path.module}/files/logrotate-app.conf")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yml")

  vars = {
    config_digest                  = sha256(data.template_file.configs.rendered)
    metadata_version               = var.metadata_version
    solomon_version                = var.solomon_agent_version
    push_client_version            = var.push_client_version
    push_client_tvm_secret         = module.yav_secret_yc_ai_tvm.secret
    push_client_tvm_secret_billing = module.yav_secret_yc_ai_tvm_billing.secret
    juggler_version                = var.juggler_version
    logrotate_version              = var.logrotate_version
    translation_server_version     = var.translation_server_version
    services_proxy_version         = var.services_proxy_version
    nvidia_dcgm_exporter_version   = var.nvidia_dcgm_exporter_version
    cadvisor_version               = var.cadvisor_version
    environment                    = var.environment
    zk_servers                     = module.constants.by_environment.zk_servers
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

module "ai_translation_service_instance_group_common" {
  // TODO: Fix that hack. Currently instance_group resource cannot work properly
  //       because it is not possible to specify platform balancer
  // source              = "../../modules/instance_group"
  source              = "../../modules/instance_group_yaml"

  // General
  folder_id           = var.yc_folder
  service_account_id  = var.yc_sa_id

  // Instance group parameters
  name                = "${var.name}-${var.environment}"
  description         = "Translation service instance group. Config path: https://a.yandex-team.ru/arc/trunk/arcadia/cloud/ai/selfhost/terraform/${var.environment}/translation" // ${element("${split("arcadia", "${abspath(path.module)}")}", -1)}"
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

  metadata = {
    user-data     = module.user_data.rendered
  }

  labels = {
    layer   = "saas"
    abc_svc = "ycai"
    env     = var.environment
  }
}

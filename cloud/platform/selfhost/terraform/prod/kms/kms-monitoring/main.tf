terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}

module "common" {
  source = "../common"
}

provider "ycp" {
  prod        = false
  ycp_profile = module.common.ycp_profile
  folder_id   = var.yc_folder
  zone        = module.common.yc_zone
}

module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = module.common.abc_group
}

# cr.yandex auth token
module "yav-secret-sa-yc-kms-cr" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01ekn6vxw8gmkt7hrxtmk4ynvh"
  value_name = "docker_auth"
}

// TVM secret for logbroker.yandex.net
module "yav-secret-yckms-tvm" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dq7mgdtspb6q82pr1jbmvw17"
  value_name = "client_secret"
}

// selfdns token
module "yav-selfdns-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dsnmbfh6kbepcsyg6npmyyg5"
  value_name = "selfdns-oauth"
}

module "yav-logbroker-iam-key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01ezg432j5khnkerv63wgr9hw4"
  value_name = "key"
}

data "template_file" "docker_json" {
  template = file("${path.module}/../common/files/docker.tpl.json")

  vars = {
    docker_auth = module.yav-secret-sa-yc-kms-cr.secret
  }
}

data "template_file" "application_yaml" {
  template = file("${path.module}/files/application.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    zone = module.common.yc_zone_suffix[element(module.common.yc_zones, count.index % length(module.common.yc_zones))]
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/configs.tpl")
  count    = var.yc_instance_group_size

  vars = {
    application_yaml = element(data.template_file.application_yaml.*.rendered, count.index)
  }
}

data "template_file" "infra_configs" {
  template = file("${path.module}/files/infra-configs.tpl")

  vars = {
    push_client_yc_logbroker_conf = file("${path.module}/files/push-client-yc-logbroker.yaml")
    solomon_agent_conf            = file("${path.module}/files/solomon-agent.conf")
    push_client_iam_key           = module.yav-logbroker-iam-key.secret
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest          = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest    = sha256(data.template_file.infra_configs.rendered)
    metadata_version       = module.common.metadata_image_version
    push_client_version    = module.common.push-client_image_version
    push_client_tvm_secret = module.yav-secret-yckms-tvm.secret
    solomon_version        = module.common.solomon_agent_image_version
    application_version    = module.common.monitoring_version
  }
}

module "kms-monitoring-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "kms-monitoring"
  hostname_prefix = "kms-monitoring"
  role_name       = "kms-monitoring"
  hostname_suffix = module.common.hostname_suffix
  osquery_tag     = module.common.osquery_tag

  zones = module.common.yc_zones

  instance_group_size = var.yc_instance_group_size

  cores_per_instance  = var.instance_cores
  memory_per_instance = var.instance_memory
  disk_per_instance   = var.instance_disk_size
  disk_type           = var.instance_disk_type
  image_id            = module.common.underlay_image_id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = data.template_file.infra_configs.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    selfdns-token = module.yav-selfdns-token.secret
  }

  labels = module.common.instance_labels

  subnets        = {}
  ipv4_addresses = []
  ipv6_addresses = []
  underlay       = true

  service_account_id = var.service_account_id
  host_group         = var.custom_host_group
}

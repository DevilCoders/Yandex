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

# OAuth token for lbkx.logbroker.yandex.net
module "yav-secret-robot-yc-kms-logbroker" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dbk6f0m1ev93vqkfd8d2ztnc"
  value_name = "logbroker_oauth_token"
}

# selfdns token for private-gpn
module "yav-selfdns-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01ecb30bn4m42dtxvxf9m5g5dg"
  value_name = "selfdns-token"
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
  template = file("${path.module}/../common/files/infra-configs.tpl")

  vars = {
    push_client_conf         = file("${path.module}/files/push-client.yaml")
    billing_push_client_conf = ""
    push_client_oauth_token  = module.yav-secret-robot-yc-kms-logbroker.secret
    # billing_push_client_conf = "${file("${path.module}/files/push-client/billing-push-client.yaml")}"
    solomon_agent_conf       = file("${path.module}/files/solomon-agent.conf")
    kms_certificate_pem      = ""
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest       = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest = sha256(data.template_file.infra_configs.rendered)
    metadata_version    = module.common.metadata_image_version
    push_client_version = module.common.push-client_image_version
    solomon_version     = module.common.solomon_agent_image_version
    application_version = module.common.monitoring_version
  }
}

module "kms-monitoring-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "kms-monitoring"
  hostname_prefix = "kms-monitoring"
  hostname_suffix = module.common.hostname_suffix
  role_name       = "kms-monitoring"
  osquery_tag     = module.common.osquery_tag

  zones                = module.common.yc_zones
  host_suffix_for_zone = module.common.yc_zone_suffix

  instance_group_size = var.yc_instance_group_size

  instance_platform_id = module.common.instance_platform_id
  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = module.common.overlay_image_id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = data.template_file.infra_configs.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    selfdns-token = module.yav-selfdns-token.secret
    user-data     = file("${path.module}/../common/files/cloud-init.sh")
  }

  labels = module.common.instance_labels

  subnets        = var.subnets
  ipv4_addresses = var.ipv4_addresses
  ipv6_addresses = var.ipv6_addresses

  service_account_id = var.service_account_id
}

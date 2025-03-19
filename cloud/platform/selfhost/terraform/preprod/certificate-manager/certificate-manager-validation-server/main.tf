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
  abc_service  = module.common.abc_group.abc_service
  abc_service_scopes = module.common.abc_group.abc_service_scopes
}

data "template_file" "application_yaml" {
  template = file("${path.module}/files/certificate-manager/application.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    zone = module.common.yc_zone_suffix[element(module.common.yc_zones, count.index % length(module.common.yc_zones))]
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/certificate-manager/configs.tpl")
  count    = var.yc_instance_group_size

  vars = {
    application_yaml = element(data.template_file.application_yaml.*.rendered, count.index)
  }
}

data "template_file" "infra_configs" {
  template = file("${path.module}/../common/files/infra-configs.tpl")

  vars = {
    solomon_agent_conf            = file("${path.module}/files/solomon-agent.conf")
    push_client_conf              = file("${path.module}/files/push-client/push-client.yaml")
    metrics_agent_conf            = file("${path.module}/files/metricsagent.yaml")
    push_client_yc_logbroker_conf = file("${path.module}/files/push-client/push-client-yc-logbroker.yaml")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/certificate-manager/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest        = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest  = sha256(data.template_file.infra_configs.rendered)
    metadata_version     = module.common.metadata_image_version
    solomon_version      = module.common.solomon_agent_image_version
    application_version  = module.common.application_version
    push_client_version  = module.common.push-client_image_version
    metricsagent_version = module.common.metricsagent_version
  }
}

module "ipv6-load-balancer" {
  source = "../modules/load-balancer"

  yc_folder = var.yc_folder

  lb_name_prefix    = "validation-server"
  ip_version        = "IPV6"
  ip_address        = var.ipv6_address
  subnets_addresses = var.subnets_ipv6_addresses
  yandex_only       = false
}

module "validation-server-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "validation-server"
  hostname_prefix = "validation"
  hostname_suffix = module.common.hostname_suffix
  role_name       = "certificate-manager-validation"
  osquery_tag     = module.common.osquery_tag

  zones = module.common.yc_zones

  instance_group_size = var.yc_instance_group_size

  cores_per_instance         = var.instance_cores
  core_fraction_per_instance = var.instance_core_fraction
  memory_per_instance        = var.instance_memory
  disk_per_instance          = var.instance_disk_size
  disk_type                  = var.instance_disk_type
  image_id                   = module.common.image_id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = data.template_file.infra_configs.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    skm = file("${path.module}/files/skm/bundle.yaml")
  }

  labels = module.common.instance_labels

  subnets        = var.subnets
  ipv4_addresses = var.ipv4_addresses
  ipv6_addresses = var.ipv6_addresses
  underlay       = false

  service_account_id   = var.service_account_id
  host_group           = module.common.host_group
  instance_platform_id = var.instance_platform_id
  security_group_ids   = var.security_group_ids
}

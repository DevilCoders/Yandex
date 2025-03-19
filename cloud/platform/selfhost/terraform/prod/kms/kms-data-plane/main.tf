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

locals {
  # Copied from modules/kubelet_instance_group_ycunderlay/main.tf
  hostnames = [
  for count in range(var.yc_instance_group_size) :
  "kms-data-${lookup(module.common.yc_zone_suffix, element(module.common.yc_zones, count))}-${floor(count / length(module.common.yc_zones)) + count % length(module.common.yc_zones) - index(module.common.yc_zones, element(module.common.yc_zones, count)) + 1}.${module.common.hostname_suffix}"
  ]

  all_hostnames   = concat(["localhost"], local.hostnames)
  envoy_endpoints = [
  for i, hostname in local.all_hostnames :
  "{ priority: ${i}, lb_endpoints: { endpoint: { address: { socket_address: { address: ${hostname}, port_value: ${var.private_api_port} } } } } }"
  ]
  lb_endpoints    = "[${join(", ", local.envoy_endpoints)}]"

  kms_addrs = [
  for hostname in local.hostnames :
  "${hostname}:${var.private_api_port}"
  ]
}

data "template_file" "application_yaml" {
  template = file("${path.module}/files/kms/application.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    private_api_port = var.private_api_port
    zone             = module.common.yc_zone_suffix[element(module.common.yc_zones, count.index % length(module.common.yc_zones))]
    discovery_addrs  = "[${join(", ", local.kms_addrs)}]"
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/kms/configs.tpl")
  count    = var.yc_instance_group_size

  vars = {
    application_yaml = element(data.template_file.application_yaml.*.rendered, count.index)
  }
}

data "template_file" "infra_configs" {
  template = file("${path.module}/files/infra-configs.tpl")

  vars = {
    billing_push_client_yc_logbroker_conf = file("${path.module}/files/push-client/billing-push-client-yc-logbroker.yaml")
    push_client_yc_logbroker_conf         = file("${path.module}/files/push-client/push-client-yc-logbroker.yaml")
    solomon_agent_conf                    = file("${path.module}/files/solomon-agent.conf")
    solomon_agent_user_metrics_conf       = file("${path.module}/files/solomon-agent-user-metrics.conf")
  }
}

data "template_file" "private_envoy_config" {
  template = file("${path.module}/files/api-gateway/private-envoy.tpl.yaml")

  vars = {
    lb_endpoints = local.lb_endpoints
  }
}

data "template_file" "api_gateway_configs" {
  template = file("${path.module}/files/api-gateway/configs.tpl")

  vars = {
    envoy_config         = file("${path.module}/files/api-gateway/envoy.yaml")
    private_envoy_config = data.template_file.private_envoy_config.rendered
    gateway_config       = file("${path.module}/files/api-gateway/gateway.yaml")
    configserver_config  = file("${path.module}/files/api-gateway/configserver.yaml")
    envoy_resources      = file("${path.module}/files/api-gateway/envoy-resources.yaml")
    gateway_services     = file("${path.module}/files/api-gateway/gateway-services.yaml")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest             = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest       = sha256(data.template_file.infra_configs.rendered)
    api_gateway_config_digest = sha256(data.template_file.api_gateway_configs.rendered)
    metadata_version          = module.common.metadata_image_version
    solomon_version           = module.common.solomon_agent_image_version
    push_client_version       = module.common.push-client_image_version
    application_version       = module.common.application_version
    config_server_version     = module.common.config_server_image_version
    api_gateway_version       = module.common.api_gateway_image_version
    envoy_version             = module.common.envoy_image_version
  }
}

module "kms-data-plane-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "kms-data"
  hostname_prefix = "kms-data"
  hostname_suffix = module.common.hostname_suffix
  role_name       = "kms-data"
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
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    skm = file("${path.module}/files/skm/bundle.yaml")
    api-gateway-configs = data.template_file.api_gateway_configs.rendered
  }

  labels = module.common.instance_labels

  subnets        = {}
  ipv4_addresses = []
  ipv6_addresses = []
  underlay       = true

  service_account_id = var.service_account_id
  host_group         = module.common.host_group
}

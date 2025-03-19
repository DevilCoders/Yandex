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

// selfdns token
module "yav-selfdns-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dsnmbfh6kbepcsyg6npmyyg5"
  value_name = "selfdns-oauth"
}

// ceritifcate for *.kms.cloud.yandex.net
module "certificate-pem" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01e8m323p914phsyxn3nkvxq0k"
  value_name = "certificate_pem"
}

// certificate for benchmark.kms.cloud.yandex.net
module "yav-secret-envoy_cert" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01fa5esce57s7b1kcpthanntpv"
  value_name = "7F0016BE7BDF407AF09572ED6800020016BE7B_certificate"
}

// certificate for benchmark.kms.cloud.yandex.net
module "yav-secret-envoy_key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01fa5esce57s7b1kcpthanntpv"
  value_name = "7F0016BE7BDF407AF09572ED6800020016BE7B_private_key"
}

locals {
  # Copied from modules/kubelet_instance_group_ycunderlay/main.tf
  hostnames = [
  for count in range(var.yc_instance_group_size) :
  "benchmark-kms-underlay-${lookup(module.common.yc_zone_suffix, element(module.common.yc_zones, count))}-${floor(count / length(module.common.yc_zones)) + count % length(module.common.yc_zones) - index(module.common.yc_zones, element(module.common.yc_zones, count)) + 1}.${module.common.hostname_suffix}"
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

data "template_file" "docker_json" {
  template = file("${path.module}/../common/files/docker.tpl.json")

  vars = {
    docker_auth = module.yav-secret-sa-yc-kms-cr.secret
  }
}

data "template_file" "application_yaml" {
  template = file("${path.module}/files/kms/application.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    private_api_port = var.private_api_port
    discovery_addrs  = "[${join(", ", local.kms_addrs)}]"
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/kms/configs.tpl")
  count    = var.yc_instance_group_size

  vars = {
    application_yaml    = element(data.template_file.application_yaml.*.rendered, count.index)
    kms_certificate_pem = module.certificate-pem.secret
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
    envoy_cert           = module.yav-secret-envoy_cert.secret
    envoy_key            = module.yav-secret-envoy_key.secret
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
    api_gateway_config_digest = sha256(data.template_file.api_gateway_configs.rendered)
    metadata_version          = module.common.metadata_image_version
    application_version       = module.common.application_version
    config_server_version     = module.common.config_server_image_version
    api_gateway_version       = module.common.api_gateway_image_version
    envoy_version             = module.common.envoy_image_version
  }
}

module "kms-data-plane-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "benchmark-kms-underlay"
  hostname_prefix = "benchmark-kms-underlay"
  hostname_suffix = module.common.hostname_suffix
  role_name       = "benchmark-kms-underlay"
  osquery_tag     = module.common.osquery_tag

  zones = module.common.yc_zones

  instance_group_size = var.yc_instance_group_size

  instance_platform_id = module.common.instance_platform_id
  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = module.common.underlay_image_id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = ""
  podmanifest          = data.template_file.podmanifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    api-gateway-configs = data.template_file.api_gateway_configs.rendered
    selfdns-token       = module.yav-selfdns-token.secret
  }

  labels = module.common.instance_labels

  subnets        = {}
  ipv4_addresses = []
  ipv6_addresses = []
  underlay       = true

  service_account_id = var.service_account_id
}

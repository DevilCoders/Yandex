terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}

provider "ycp" {
  prod        = false
  ycp_profile = var.ycp_profile
  folder_id   = var.yc_folder
  zone        = var.yc_zone
}

module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "yckms"
}

locals {
  # Copied from modules/kubelet_instance_group_ycp/main.tf
  hostnames = [
  for count in range(var.yc_instance_group_size) :
  "lockbox-control-${lookup(var.yc_zone_suffix, element(var.yc_zones, count))}-${floor(count / length(var.yc_zones)) + count % length(var.yc_zones) - index(var.yc_zones, element(var.yc_zones, count)) + 1}.${var.hostname_suffix}"
  ]

  envoy_endpoints = [
  for hostname in local.hostnames :
  "{ endpoint: { address: { socket_address: { address: ${hostname}, port_value: ${var.private_api_port} } } } }"
  ]

  kms_addrs = [
  for hostname in local.hostnames :
  "${hostname}:${var.private_api_port}"
  ]
}

data "template_file" "application_yaml" {
  template = file("${path.module}/files/lockbox/application.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    private_api_port = var.private_api_port
    zone             = var.yc_zone_suffix[element(var.yc_zones, count.index % length(var.yc_zones))]
    discovery_addrs  = "[${join(", ", local.kms_addrs)}]"
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/lockbox/configs.tpl")
  count    = var.yc_instance_group_size

  vars = {
    application_yaml = element(data.template_file.application_yaml.*.rendered, count.index)
  }
}

data "template_file" "infra_configs" {
  template = file("${path.module}/files/infra-configs.tpl")

  vars = {
    push_client_yc_logbroker_conf = file("${path.module}/files/push-client/push-client-yc-logbroker.yaml")
    solomon_agent_conf            = file("${path.module}/files/solomon-agent.conf")
  }
}

data "template_file" "private_envoy_config" {
  template = file("${path.module}/files/api-gateway/private-envoy.tpl.yaml")

  vars = {
    lb_endpoints = "[${join(", ", local.envoy_endpoints)}]"
  }
}

data "template_file" "api_gateway_configs" {
  template = file("${path.module}/files/api-gateway/configs.tpl")

  vars = {
    yandex_internal_root_ca = file("${path.module}/../../../common/allCAs.pem")
    envoy_config            = file("${path.module}/files/api-gateway/envoy.yaml")
    private_envoy_config    = data.template_file.private_envoy_config.rendered
    gateway_config          = file("${path.module}/files/api-gateway/gateway.yaml")
    configserver_config     = file("${path.module}/files/api-gateway/configserver.yaml")
    envoy_resources         = file("${path.module}/files/api-gateway/envoy-resources.yaml")
    gateway_services        = file("${path.module}/files/api-gateway/gateway-services.yaml")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest             = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest       = sha256(data.template_file.infra_configs.rendered)
    api_gateway_config_digest = sha256(data.template_file.api_gateway_configs.rendered)
    metadata_version          = var.metadata_image_version
    solomon_version           = var.solomon_agent_image_version
    push_client_version       = var.push-client_image_version
    application_version       = var.application_version
    config_server_version     = var.config_server_image_version
    api_gateway_version       = var.api_gateway_image_version
    envoy_version             = var.envoy_image_version
  }
}

module "lockbox-control-plane-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "lockbox-control"
  hostname_prefix = "lockbox-control"
  hostname_suffix = var.hostname_suffix
  role_name       = "lockbox-control"
  osquery_tag     = "ycloud-svc-lockbox"

  zones = var.yc_zones

  instance_group_size = var.yc_instance_group_size

  instance_platform_id = var.instance_platform_id
  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = var.image_id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = data.template_file.infra_configs.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    skm = file("${path.module}/files/skm/bundle.yaml")
    api-gateway-configs = data.template_file.api_gateway_configs.rendered
  }

  labels = {
    layer   = "paas"
    abc_svc = "yckms"
    env     = "prod"
  }

  subnets        = var.subnets
  ipv4_addresses = var.ipv4_addresses
  ipv6_addresses = var.ipv6_addresses
  underlay       = false

  service_account_id = var.service_account_id
  host_group         = var.host_group
  security_group_ids = var.security_group_ids
}

#module "ipv4-target-group" {
#  source = "../modules/target-group"
#
#  name    = "control-plane-ipv4-target-group"
#  token    = "${var.yc_token}"
#  folder   = "${var.yc_folder}"
#  zone     = "${var.yc_zone}"
#
#  subnets_addresses = "${var.subnets_ipv4_addresses}"
#}

module "ipv6-target-group" {
  source = "../modules/target-group"

  name        = "control-plane-ipv6-target-group"
  ycp_profile = var.ycp_profile
  folder      = var.yc_folder
  zone        = var.yc_region

  subnets_addresses = var.subnets_ipv6_addresses
}

module "private-ipv6-load-balancer" {
  source = "../modules/load-balancer"

  name        = "control-plane-private-ipv6-load-balancer"
  ycp_profile = var.ycp_profile
  folder      = var.yc_folder
  zone        = var.yc_region

  ip_version        = "IPV6"
  ip_address        = "2a0d:d6c0:0:1c::28b"
  yandex_only       = true
  port              = var.lb_private_api_port
  target_group_id   = module.ipv6-target-group.id
  health_check_path = var.health_check_path
  health_check_port = var.private_health_check_port
}

module "public-ipv6-load-balancer" {
  source = "../modules/load-balancer"

  name        = "control-plane-public-ipv6-load-balancer"
  ycp_profile = var.ycp_profile
  folder      = var.yc_folder
  zone        = var.yc_region

  ip_version        = "IPV6"
  ip_address        = "2a0d:d6c0:0:1c::2ae"
  yandex_only       = true
//  yandex_only       = false
  port              = var.lb_public_api_port
  target_group_id   = module.ipv6-target-group.id
  health_check_path = var.health_check_path
  health_check_port = var.health_check_port
}

module "ipv4-target-group" {
  source = "../modules/target-group"

  name        = "control-plane-ipv4-target-group"
  ycp_profile = var.ycp_profile
  folder      = var.yc_folder
  zone        = var.yc_region

  subnets_addresses = var.subnets_ipv4_addresses
}

module "public-ipv4-load-balancer" {
  source = "../modules/load-balancer"

  name        = "control-plane-public-ipv4-load-balancer"
  ycp_profile = var.ycp_profile
  folder      = var.yc_folder
  zone        = var.yc_region

  ip_version        = "IPV4"
  ip_address        = "84.201.168.69"
  yandex_only       = false
  port              = var.lb_public_api_port
  target_group_id   = module.ipv4-target-group.id
  health_check_path = var.health_check_path
  health_check_port = var.health_check_port
}

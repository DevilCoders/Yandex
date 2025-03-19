terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}

provider "ycp" {
  prod      = var.ycp_prod
  token     = var.yc_token
  folder_id = var.yc_folder
  zone      = var.yc_zone
}

module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "yckms"
}

// robot-yc-kms registry.yandex.net auth token
module "yav-secret-robot-yc-kms-docker" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dbk6f0m1ev93vqkfd8d2ztnc"
  value_name = "docker_auth_registry_yandex_net"
}

// TVM secret for logbroker.yandex.net
module "yav-secret-yckms-tvm" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dq7mgdtspb6q82pr1jbmvw17"
  value_name = "client_secret"
}

# selfdns token
module "yav-selfdns-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dsnmbfh6kbepcsyg6npmyyg5"
  value_name = "selfdns-oauth"
}

module "certificate-pem" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01eknbte8xcs8fbcgec8pwccr2"
  value_name = "certificate_pem"
}

module "yav-secret-envoy_cert" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01ekz0h2e01ctec3t25493qwhq"
  value_name = "5365D48584BC399CE4DBDFBFF7B7BA0C_certificate"
}

module "yav-secret-envoy_key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01ekz0h2e01ctec3t25493qwhq"
  value_name = "5365D48584BC399CE4DBDFBFF7B7BA0C_private_key"
}

module "yav-secret-private_envoy_cert" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01ekwhj7f5qdfd5ak0kkc008zg"
  value_name = "7F0010683FBC39CC75D4B189EF00020010683F_certificate"
}

module "yav-secret-private_envoy_key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01ekwhj7f5qdfd5ak0kkc008zg"
  value_name = "7F0010683FBC39CC75D4B189EF00020010683F_private_key"
}

locals {
  # Copied from modules/kubelet_instance_group_ycp/main.tf
  hostnames = [
    for count in range(var.yc_instance_group_size) :
    "lockbox-data-${lookup(var.yc_zone_suffix, element(var.yc_zones, count))}-${floor(count / length(var.yc_zones)) + count % length(var.yc_zones) - index(var.yc_zones, element(var.yc_zones, count)) + 1}.${var.hostname_suffix}"
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

data "template_file" "docker_json" {
  template = file("${path.module}/../common/docker.tpl.json")

  vars = {
    docker_auth = module.yav-secret-robot-yc-kms-docker.secret
  }
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
    push_client_conf         = file("${path.module}/files/push-client/push-client.yaml")
    #billing_push_client_conf = ""
    solomon_agent_conf       = file("${path.module}/files/solomon-agent.conf")
    lockbox_certificate_pem      = module.certificate-pem.secret
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
    envoy_cert              = module.yav-secret-envoy_cert.secret
    envoy_key               = module.yav-secret-envoy_key.secret
    private_envoy_cert      = module.yav-secret-private_envoy_cert.secret
    private_envoy_key       = module.yav-secret-private_envoy_key.secret
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
    push_client_tvm_secret    = module.yav-secret-yckms-tvm.secret
    application_version       = var.application_version
    config_server_version     = var.config_server_image_version
    api_gateway_version       = var.api_gateway_image_version
    envoy_version             = var.envoy_image_version
  }
}

module "lockbox-data-plane-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "lockbox-load-test"
  hostname_prefix = "lockbox-load-test"
  hostname_suffix = var.hostname_suffix
  role_name       = "lockbox-load-test"
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
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    api-gateway-configs = data.template_file.api_gateway_configs.rendered
    selfdns-token       = module.yav-selfdns-token.secret
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
  #security_group_ids = "${var.security_group_ids}"
}

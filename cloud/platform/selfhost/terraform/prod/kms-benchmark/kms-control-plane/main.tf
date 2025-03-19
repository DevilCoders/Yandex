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

// certificate for cpl.benchmark.kms.cloud.yandex.net
module "yav-secret-envoy_cert" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01fa5jgbd62b0ngm2ytpshe4gb"
  value_name = "7F0016BEF4333DBB9ACE7D4EEB00020016BEF4_certificate"
}

// certificate for cpl.benchmark.kms.cloud.yandex.net
module "yav-secret-envoy_key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01fa5jgbd62b0ngm2ytpshe4gb"
  value_name = "7F0016BEF4333DBB9ACE7D4EEB00020016BEF4_private_key"
}

locals {
  # Copied from modules/kubelet_instance_group_ycp/main.tf
  hostnames = [
  for count in range(var.yc_instance_group_size) :
  "benchmark-kms-control-${lookup(module.common.yc_zone_suffix, element(module.common.yc_zones, count))}-${floor(count / length(module.common.yc_zones)) + count % length(module.common.yc_zones) - index(module.common.yc_zones, element(module.common.yc_zones, count)) + 1}.${module.common.hostname_suffix}"
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
    instance_num     = count.index + 1
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
  template = file("${path.module}/files/private-envoy/private-envoy.tpl.yaml")

  vars = {
    lb_endpoints = local.lb_endpoints
  }
}

data "template_file" "private_envoy_configs" {
  template = file("${path.module}/files/private-envoy/configs.tpl")

  vars = {
    private_envoy_config = data.template_file.private_envoy_config.rendered
    envoy_cert           = module.yav-secret-envoy_cert.secret
    envoy_key            = module.yav-secret-envoy_key.secret
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest               = sha256(element(data.template_file.configs.*.rendered, count.index))
    private_envoy_config_digest = sha256(data.template_file.private_envoy_configs.rendered)
    metadata_version            = module.common.metadata_image_version
    application_version         = module.common.application_version
    envoy_version               = module.common.envoy_image_version
  }
}

module "kms-control-plane-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "benchmark-kms-control"
  hostname_prefix = "benchmark-kms-control"
  hostname_suffix = module.common.hostname_suffix
  role_name       = "benchmark-kms-control"
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
  infra-configs        = ""
  podmanifest          = data.template_file.podmanifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    private-envoy-configs = data.template_file.private_envoy_configs.rendered
    selfdns-token         = module.yav-selfdns-token.secret
  }

  labels = module.common.instance_labels

  subnets        = var.subnets
  ipv4_addresses = var.ipv4_addresses
  ipv6_addresses = var.ipv6_addresses

  service_account_id = var.service_account_id
}

resource "ycp_load_balancer_target_group" "load-balancer-tg" {
  name      = "control-plane-tg"
  region_id = module.common.yc_region

  dynamic "target" {
    for_each = range(var.yc_instance_group_size)
    iterator = count

    content {
      address   = element(var.ipv6_addresses, count.value)
      subnet_id = lookup(var.subnets, element(module.common.yc_zones, count.value))
    }
  }

  labels = module.common.instance_labels
}

resource "ycp_load_balancer_network_load_balancer" "load-balancer" {
  name      = "control-plane-lb"
  region_id = module.common.yc_region
  type      = "EXTERNAL"

  labels = module.common.instance_labels

  attached_target_group {
    health_check {
      name                = "healthcheck"
      timeout             = "1s"
      interval            = "5s"
      healthy_threshold   = 3
      unhealthy_threshold = 2
      http_options {
        path = var.health_check_path
        port = var.health_check_port
      }
    }

    target_group_id = ycp_load_balancer_target_group.load-balancer-tg.id
  }

  listener_spec {
    name        = "private-listener"
    external_address_spec {
      ip_version  = "IPV6"
      yandex_only = true
      address     = var.lb_address
    }
    port        = var.lb_private_api_port
    target_port = var.lb_private_api_port
    protocol    = "TCP"
  }
}

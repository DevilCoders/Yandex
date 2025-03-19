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
  folder_id   = module.common.folder_id
  zone        = module.common.yc_zone
}

module "ssh-keys" {
  source             = "../../../modules/ssh-keys"
  yandex_token       = var.yandex_token
  abc_service        = module.common.abc_group.abc_service
  abc_service_scopes = module.common.abc_group.abc_service_scopes
}

locals {
  backend_private_api_port = "9443"
}

module "hostnames" {
  source = "../../../modules/kubelet_instance_group_ycp_v2_hostnames"

  instance_group_size  = var.instance_group_size
  api_port             = local.backend_private_api_port
  hostname_prefix      = var.hostname_prefix
  hostname_suffix      = module.common.hostname_suffix
  zones                = module.common.yc_zones
  host_suffix_for_zone = module.common.yc_zone_suffix
}

data "template_file" "application_yaml" {
  template = file("${path.module}/files/certificate-manager/application.tpl.yaml")
  count    = var.instance_group_size

  vars = {
    api_port = local.backend_private_api_port
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/certificate-manager/configs.tpl")
  count    = var.instance_group_size

  vars = {
    application_yaml        = element(data.template_file.application_yaml.*.rendered, count.index)
    yandex_internal_root_ca = file("${path.module}/../../../common/allCAs.pem")
  }
}

data "template_file" "infra_configs" {
  template = file("${path.module}/files/infra-configs.tpl")

  vars = {
    solomon_agent_conf = file("${path.module}/files/solomon-agent.conf")
  }
}

module "api-locallb" {
  source = "../../../modules/api-locallb"

  endpoint                = var.public_endpoint
  private_endpoint        = var.private_endpoint
  api_port                = var.api_port
  hc_port                 = var.hc_port
  private_api_port        = var.private_api_port
  private_hc_port         = var.private_hc_port
  region                  = module.common.yc_region
  zones                   = module.common.yc_zones
  id_prefix               = var.id_prefix
  operation_service_id    = "yandex.cloud.priv.certificatemanager.v1.OperationService"
  service_id              = "yandex.cloud.certificatemanager.v1.CertificateContentService"
  metadata_version        = module.common.metadata_image_version
  configserver_pod_memory = var.configserver_pod_memory
  gateway_pod_memory      = var.gateway_pod_memory
  envoy_heap_memory       = var.envoy_heap_memory
  envoy_pod_memory        = var.envoy_pod_memory
  envoy_endpoints_string  = module.hostnames.envoy_endpoints_string
  per_try_timeout         = "5s"
  private_api_cert_crt    = "/run/api/envoy/private-api.crt"
  private_api_cert_key    = "/run/api/envoy/private-api.key"
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.instance_group_size

  vars = {
    config_digest       = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest = sha256(data.template_file.infra_configs.rendered)
    metadata_version    = module.common.metadata_image_version
    solomon_version     = module.common.solomon_agent_image_version
    application_version = module.common.application_version
    java_pod_memory     = var.java_pod_memory
    java_heap_memory    = var.java_heap_memory
    api_podmanifest     = module.api-locallb.api_podmanifest
  }
}

module "certificate-manager-dpl-instance-group" {
  source               = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix          = var.hostname_prefix
  hostname_prefix      = var.hostname_prefix
  hostname_suffix      = module.common.hostname_suffix
  fqdn_hostname_suffix = ""
  role_name            = var.role_label
  osquery_tag          = module.common.osquery_tag

  zones                = module.common.yc_zones
  host_suffix_for_zone = module.common.yc_zone_suffix
  dns_zone_id          = module.common.dns_zone_id

  instance_group_size = var.instance_group_size

  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = module.common.image_id
  instance_platform_id = module.common.instance_platform_id
  placement_group_id   = module.common.dpl-placement-group-id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = data.template_file.infra_configs.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    user-data   = module.common.cloud-init-sh
    skm         = file("${path.module}/files/skm/bundle.yaml")
    api-configs = module.api-locallb.api_configs_metadata
  }

  labels = module.common.instance_labels

  subnets        = module.common.subnets
  ipv4_addresses = []
  ipv6_addresses = []
  underlay       = false

  service_account_id = module.common.service_account_id
}

resource "ycp_vpc_inner_address" "dpl-load-balancer-address" {
  name = "dpl-address"

  external_address_spec {
    ip_version  = "IPV6"
    yandex_only = true
    # Use whatever zone from our region
    zone_id     = module.common.yc_zone
  }
  ephemeral = false

  labels = module.common.instance_labels
}

resource "ycp_load_balancer_target_group" "dpl-load-balancer-tg" {
  name      = "dpl-tg"
  region_id = module.common.yc_region

  dynamic "target" {
    for_each = range(var.instance_group_size)
    iterator = count

    content {
      address   = element(module.certificate-manager-dpl-instance-group.all_instance_ipv6_addresses, count.value)
      subnet_id = lookup(module.common.subnets, element(module.common.yc_zones, count.value))
    }
  }

  labels = module.common.instance_labels
}

resource "ycp_load_balancer_network_load_balancer" "dpl-load-balancer" {
  name      = "dpl-lb"
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
        path = "/"
        # We use private HC port here, because we did not have the network access originally. Change for PROD!
        port = var.private_hc_port
      }
    }

    target_group_id = ycp_load_balancer_target_group.dpl-load-balancer-tg.id
  }

  listener_spec {
    name        = "public-listener"
    external_address_spec {
      ip_version  = "IPV6"
      yandex_only = false
      address     = ycp_vpc_inner_address.dpl-load-balancer-address.address
    }
    port        = var.api_port
    target_port = var.api_port
    protocol    = "TCP"
  }

  listener_spec {
    name        = "private-listener"
    external_address_spec {
      ip_version  = "IPV6"
      yandex_only = false
      address     = ycp_vpc_inner_address.dpl-load-balancer-address.address
    }
    port        = var.private_api_port
    target_port = var.private_api_port
    protocol    = "TCP"
  }
}

resource "ycp_dns_dns_record_set" "dpl-load-balancer-fqdn" {
  name    = var.endpoint_name
  zone_id = module.common.dns_zone_id
  type    = "AAAA"
  ttl     = 300
  data    = [ycp_vpc_inner_address.dpl-load-balancer-address.address]
}

// public balancer

resource "ycp_vpc_address" "dpl-load-balancer-v4-address" {

  lifecycle {
    prevent_destroy = true
    ignore_changes  = [reserved, labels]
  }

  name = "dpl-v4-address"
  folder_id = module.common.folder_id

  external_ipv4_address_spec {
    zone_id = module.common.yc_zone
  }
}

resource "ycp_load_balancer_target_group" "dpl-load-balancer-v4-tg" {
  name      = "dpl-v4-tg"
  region_id = module.common.yc_region

  dynamic "target" {
    for_each = range(var.instance_group_size)
    iterator = count

    content {
      address   = element(module.certificate-manager-dpl-instance-group.all_instance_ipv4_addresses, count.value)
      subnet_id = lookup(module.common.subnets, element(module.common.yc_zones, count.value))
    }
  }

  labels = module.common.instance_labels
}

resource "ycp_load_balancer_network_load_balancer" "dpl-v4-load-balancer" {
  name      = "dpl-v4-lb"
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
        path = "/"
        # We use private HC port here, because we did not have the network access originally. Change for PROD!
        port = var.private_hc_port
      }
    }

    target_group_id = ycp_load_balancer_target_group.dpl-load-balancer-v4-tg.id
  }

  listener_spec {
    name        = "public-listener"
    external_address_spec {
      ip_version  = "IPV4"
      yandex_only = false
      address     = ycp_vpc_address.dpl-load-balancer-v4-address.external_ipv4_address[0].address
    }
    port        = var.api_port
    target_port = var.api_port
    protocol    = "TCP"
  }
}

resource "ycp_dns_dns_record_set" "dpl-public-v4-load-balancer-fqdn" {
  name    = var.public_endpoint_name
  zone_id = module.common.dns_zone_id
  type    = "A"
  ttl     = 300
  data    = [ycp_vpc_address.dpl-load-balancer-v4-address.external_ipv4_address[0].address]
}

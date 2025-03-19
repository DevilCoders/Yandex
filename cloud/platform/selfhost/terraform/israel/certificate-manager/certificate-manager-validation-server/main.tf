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
  template = file("${path.module}/files/infra-configs.tpl")

  vars = {
    solomon_agent_conf = file("${path.module}/files/solomon-agent.conf")
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
  }
}

//module "ipv6-load-balancer" {
//  source = "../modules/load-balancer"
//
//  yc_folder = var.yc_folder
//
//  lb_name_prefix    = "validation-server"
//  ip_version        = "IPV6"
//  ip_address        = var.ipv6_address
//  subnets_addresses = var.subnets_ipv6_addresses
//  yandex_only       = false
//}

module "hostnames" {
  source = "../../../modules/kubelet_instance_group_ycp_v2_hostnames"

  instance_group_size  = var.yc_instance_group_size
  api_port             = 443
  hostname_prefix      = var.hostname_prefix
  hostname_suffix      = module.common.hostname_suffix
  zones                = module.common.yc_zones
  host_suffix_for_zone = module.common.yc_zone_suffix
}

module "validation-server-instance-group" {
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

  instance_group_size = var.yc_instance_group_size

  cores_per_instance   = var.instance_cores
  core_fraction_per_instance = var.instance_core_fraction
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = module.common.image_id
  instance_platform_id = module.common.instance_platform_id
  placement_group_id   = module.common.validation-placement-group-id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = data.template_file.infra_configs.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    skm         = file("${path.module}/files/skm/bundle.yaml")
  }

  labels = module.common.instance_labels

  subnets        = module.common.subnets
  ipv4_addresses = []
  ipv6_addresses = []
  underlay       = false

  service_account_id = module.common.service_account_id

}

resource "ycp_vpc_inner_address" "validation-load-balancer-address" {
  name = "validation-address"

  external_address_spec {
    ip_version  = "IPV6"
    yandex_only = true
    # Use whatever zone from our region
    zone_id     = module.common.yc_zone
  }
  ephemeral = false

  labels = module.common.instance_labels
}

resource "ycp_load_balancer_target_group" "validation-load-balancer-tg" {
  name      = "validation-tg"
  region_id = module.common.yc_region

  dynamic "target" {
    for_each = range(var.yc_instance_group_size)
    iterator = count

    content {
      address   = element(module.validation-server-instance-group.all_instance_ipv6_addresses, count.value)
      subnet_id = lookup(module.common.subnets, element(module.common.yc_zones, count.value))
    }
  }

  labels = module.common.instance_labels
}

resource "ycp_load_balancer_network_load_balancer" "validation-load-balancer" {
  name      = "validation-lb"
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
        port = 9982
      }
    }

    target_group_id = ycp_load_balancer_target_group.validation-load-balancer-tg.id
  }

  listener_spec {
    name        = "public-listener"
    external_address_spec {
      ip_version  = "IPV6"
      yandex_only = false
      address     = ycp_vpc_inner_address.validation-load-balancer-address.address
    }
    port        = 443
    target_port = 443
    protocol    = "TCP"
  }

//  listener_spec {
//    name        = "private-listener"
//    external_address_spec {
//      ip_version  = "IPV6"
//      yandex_only = false
//      address     = ycp_vpc_inner_address.dpl-load-balancer-address.address
//    }
//    port        = var.private_api_port
//    target_port = var.private_api_port
//    protocol    = "TCP"
//  }
}

resource "ycp_dns_dns_record_set" "validation-load-balancer-fqdn" {
  name    = var.endpoint_name
  zone_id = module.common.dns_zone_id
  type    = "AAAA"
  ttl     = 300
  data    = [ycp_vpc_inner_address.validation-load-balancer-address.address]
}

// public balancer

resource "ycp_vpc_address" "validation-load-balancer-v4-address" {

  lifecycle {
    prevent_destroy = true
    ignore_changes  = [reserved, labels]
  }

  name = "validation-v4-address"
  folder_id = module.common.folder_id

  external_ipv4_address_spec {
    zone_id = module.common.yc_zone
  }
}

resource "ycp_load_balancer_target_group" "validation-load-balancer-v4-tg" {
  name      = "validation-v4-tg"
  region_id = module.common.yc_region

  dynamic "target" {
    for_each = range(var.yc_instance_group_size)
    iterator = count

    content {
      address   = element(module.validation-server-instance-group.all_instance_ipv4_addresses, count.value)
      subnet_id = lookup(module.common.subnets, element(module.common.yc_zones, count.value))
    }
  }

  labels = module.common.instance_labels
}

resource "ycp_load_balancer_network_load_balancer" "validation-v4-load-balancer" {
  name      = "validation-v4-lb"
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
        port = 9982
      }
    }

    target_group_id = ycp_load_balancer_target_group.validation-load-balancer-v4-tg.id
  }

  listener_spec {
    name        = "public-listener"
    external_address_spec {
      ip_version  = "IPV4"
      yandex_only = false
      address     = ycp_vpc_address.validation-load-balancer-v4-address.external_ipv4_address[0].address
    }
    port        = 443
    target_port = 443
    protocol    = "TCP"
  }
}

resource "ycp_dns_dns_record_set" "validation-public-v4-load-balancer-fqdn" {
  name    = var.public_endpoint_name
  zone_id = module.common.dns_zone_id
  type    = "A"
  ttl     = 300
  data    = [ycp_vpc_address.validation-load-balancer-v4-address.external_ipv4_address[0].address]
}

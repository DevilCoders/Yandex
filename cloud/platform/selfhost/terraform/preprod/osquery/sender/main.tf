terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}

provider "ycp" {
  ycp_profile = var.ycp_profile
  folder_id   = var.yc_folder
  zone        = var.yc_zone
}

module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = var.abc_group
}

data "template_file" "conf_yaml" {
  template = file("${path.module}/files/conf.tpl.yaml")

  vars = {
    clickhouse_hosts       = var.clickhouse_hosts
    s3_endpoint            = var.s3_endpoint
    s3_access_key_id       = var.s3_access_key_id
    ch_sender_batch_memory = var.ch_sender_batch_memory
    s3_sender_batch_memory = var.s3_sender_batch_memory
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/configs.tpl")
  count    = var.yc_instance_group_size

  vars = {
    conf_yaml = data.template_file.conf_yaml.rendered
  }
}

data "template_file" "solomon_agent_conf" {
  template = file("${path.module}/files/solomon-agent.tpl.conf")

  vars = {
    solomon_storage_limit = var.solomon_storage_limit
    healthcheck_port      = var.healthcheck_port
  }
}

data "template_file" "infra_configs" {
  template = file("${path.module}/files/infra-configs.tpl")

  vars = {
    solomon_agent_conf = data.template_file.solomon_agent_conf.rendered
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest        = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest  = sha256(element(data.template_file.infra_configs.*.rendered, count.index))
    metadata_version     = var.metadata_image_version
    api_port             = var.api_port
    healthcheck_port     = var.healthcheck_port
    sender_version       = var.sender_version
    sender_registry      = var.sender_registry
    sender_memory_limit  = var.sender_memory_limit
    solomon_version      = var.solomon_agent_image_version
    solomon_memory_limit = var.solomon_memory_limit
  }
}

module "sender-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "sender"
  hostname_prefix = "sender"
  hostname_suffix = var.hostname_suffix
  role_name       = "sender"
  osquery_tag     = ""

  zones = var.yc_zones

  instance_group_size = var.yc_instance_group_size

  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = var.image_id
  instance_platform_id = var.instance_platform_id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = data.template_file.infra_configs.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    skm = file("${path.module}/files/skm/skm.yaml")
  }

  subnets        = var.subnets
  ipv4_addresses = var.ipv4_addresses
  ipv6_addresses = var.ipv6_addresses
  underlay       = false

  host_group         = "${var.host_group}"
  service_account_id = var.service_account_id
  security_group_ids = "${var.security_group_ids}"
}

resource "ycp_load_balancer_target_group" "load-balancer-tg" {
  name      = "sender-tg"
  region_id = var.yc_region

  dynamic "target" {
    for_each = range(var.yc_instance_group_size)
    iterator = count

    content {
      address   = element(var.ipv6_addresses, count.value)
      subnet_id = lookup(var.subnets, element(var.yc_zones, count.value))
    }
  }
}

resource "ycp_load_balancer_network_load_balancer" "load-balancer" {
  name      = "sender-lb"
  region_id = var.yc_region
  type      = "EXTERNAL"

  attached_target_group {
    health_check {
      name                = "healthcheck"
      timeout             = "1s"
      interval            = "5s"
      healthy_threshold   = 3
      unhealthy_threshold = 2
      http_options {
        port = var.healthcheck_port
        path = "/"
      }
    }

    target_group_id = ycp_load_balancer_target_group.load-balancer-tg.id
  }

  listener_spec {
    name     = "listener"
    external_address_spec {
      ip_version  = "IPV6"
      yandex_only = true
      address     = var.lb_address
    }
    port     = var.api_port
    protocol = "TCP"
  }
}

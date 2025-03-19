locals {
  instances = flatten([
  for i in range(var.instances_amount) : {
      hostname     = "gsoc-${i/length(var.yc_zones)+1}.${var.yc_zone_suffixes[var.yc_zones[i % length(var.yc_zones)]]}"
      fqdn         = "gsoc-${i/length(var.yc_zones)+1}.${var.yc_zone_suffixes[var.yc_zones[i % length(var.yc_zones)]]}.${var.dns_zone}"
      az           = var.yc_zones[i % length(var.yc_zones)]
      subnet_id    = var.subnets[var.yc_zones[i % length(var.yc_zones)]]
      ipv4_address = var.ipv4_addresses[i]
      ipv6_address = var.ipv6_addresses[i]
    }
  ])

  instances_map = {
  for instance in local.instances : replace(instance.hostname, ".", "-") => instance
  }
}

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
  zone        = var.ycp_zone
}

module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = var.abc_group
}

data "template_file" "conf_yaml" {
  template = file("${path.module}/files/conf.tpl.yaml")

  vars = {
    # clickhouse_hosts       = var.clickhouse_hosts
    # s3_endpoint            = var.s3_endpoint
    # s3_access_key_id       = var.s3_access_key_id
    # kinesis_endpoint       = var.kinesis_endpoint
    # kinesis_access_key_id  = var.kinesis_access_key_id
    # ch_sender_batch_memory = var.ch_sender_batch_memory
    # s3_sender_batch_memory = var.s3_sender_batch_memory
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/configs.tpl")
  count    = var.instances_amount

  vars = {
    conf_yaml = data.template_file.conf_yaml.rendered
  }
}


resource "ycp_compute_instance" "gsoc_robot_host" {
  for_each    = local.instances_map
  zone_id     = each.value.az
  platform_id = var.platform_id
  name        = each.key
  fqdn        = each.value.fqdn
  service_account_id = var.service_account_id

  placement_policy {
    host_group = "service"
  }

  resources {
    cores         = var.instance_cores
    core_fraction = var.instance_core_fraction
    memory        = var.instance_memory
  }

  boot_disk {
    disk_spec {
      name        = "${each.key}-boot-disk"
      description = "${each.key} boot disk"
      image_id    = var.image_id
      type_id     = var.instance_disk_type
      size        = var.instance_disk_size
      labels      = var.labels
    }
    device_name = "boot"
  }

  network_interface {
    subnet_id = each.value.subnet_id
    primary_v6_address {
      address = each.value.ipv6_address
    }
    primary_v4_address {
      address = each.value.ipv4_address
    }
  }

  metadata = {
    user-data = templatefile("${path.module}/files/cloud-init.yaml", {
      fqdn = each.value.fqdn
      gsoc-robot-config = templatefile("${path.module}/files/application.config.yaml", {
        db_host = var.database.host
        db_port = var.database.port
        db_name = var.database.name
        db_user = var.database.user

        smtp_addr = var.smtp.addr
        smtp_user = var.smtp.user

        kinesis_endpoint = var.kinesis.endpoint
        kinesis_region = var.kinesis.region
        kinesis_stream_name = var.kinesis.stream_name
        kinesis_key_id = var.kinesis.key_id
      })
    })

    skm = file("${path.module}/files/skm/skm.yaml")

    ssh-keys = module.ssh-keys.ssh-keys
    podmanifest = ""
  }

  labels = var.labels
}

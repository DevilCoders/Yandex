terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}

locals {
  target_zone = var.zone
  host_suffix = lookup(var.host_suffix_for_zone, local.target_zone)
  hostname    = "${var.hostname_prefix}${var.name}${var.hostname_suffix}-${local.host_suffix}${var.host_index}"
  fqdn        = "${local.hostname}.${var.nsdomain}"

  logging_endpoint = lookup(var.logging_endpoint_for_installation, var.installation)

  metadata_ssh = var.ssh-keys != "" ? { ssh-keys = var.ssh-keys } : {}
  metadata_module = {
    osquery_tag                = var.osquery_tag
    fluentbit_env              = <<-EOF
      YC_ENDPOINT=${local.logging_endpoint}
      YC_GROUP=${var.logging_group_id}
    EOF
    k8s-runtime-bootstrap-yaml = <<-EOF
      write_files:
        - path: /etc/yc/installation
          metadata_key: installation
        - path: /etc/td-agent-bit/td-agent-bit.env
          metadata_key: fluentbit_env
        - path: /etc/resolv.dnsmasq
          metadata_key: dnsmasq-nameservers
    EOF
    dnsmasq-nameservers        = <<-EOF
      nameserver 2a02:6b8::1:1
    EOF
    user-data                  = data.template_file.user-meta.rendered
    shortname                  = local.hostname
    nsdomain                   = var.nsdomain
    installation               = var.installation
    enable-oslogin             = "${var.oslogin}"
  }

  metadata_full = merge(local.metadata_ssh, local.metadata_module, var.metadata)

  labels_module = {
    skip_update_ssh_keys = var.skip_update_ssh_keys
    abc_svc              = "ycincome"
  }

  labels_full = merge(local.labels_module, var.labels)

  dns_zone_list          = var.dns_zone_id != "" ? [var.dns_zone_id] : []
  underlay_dns_zone_list = var.skip_underlay ? [] : local.dns_zone_list
  overlay_dns_zone_list  = var.skip_underlay ? local.dns_zone_list : []
}

resource "ycp_compute_instance" "inst" {
  name                      = local.hostname
  description               = var.instance_description
  folder_id                 = var.folder_id
  zone_id                   = local.target_zone
  hostname                  = local.hostname
  service_account_id        = var.service_account_id
  allow_stopping_for_update = true
  fqdn                      = local.fqdn

  labels = local.labels_full

  metadata = local.metadata_full

  platform_id = var.instance_platform_id
  resources {
    cores         = var.cores_per_instance
    core_fraction = var.core_fraction_per_instance
    memory        = var.memory_per_instance
  }

  boot_disk {
    auto_delete = true
    disk_spec {
      image_id = var.image_id
      size     = var.disk_per_instance
      type_id  = var.disk_type
    }
  }

  network_interface {
    subnet_id = var.subnet
    primary_v4_address {}
    primary_v6_address {
      dynamic "dns_record" {
        # for_each = local.overlay_dns_zone_list
        for_each = local.dns_zone_list
        content {
          fqdn        = local.hostname
          dns_zone_id = dns_record.value
        }
      }
    }
    security_group_ids = var.security_group_ids
  }

  dynamic "underlay_network" { # skip overall underlay network
    for_each = var.skip_underlay ? [] : [1]
    content {
      network_name = "underlay-v6"
      # dynamic "ipv6_dns_record" { # skip dns for underlay network
      #   for_each = local.underlay_dns_zone_list
      #   content {
      #     fqdn        = local.hostname
      #     dns_zone_id = ipv6_dns_record.value
      #   }
      # }
    }
  }
}

data "template_file" "user-meta" {
  template = file("${path.module}/templates/user-data.tpl")
  vars = {
    netplan_config = jsonencode(local.netplan_config)
  }
}

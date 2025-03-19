resource "ycp_compute_image" "web_server" {
  folder_id = var.folder_id

  name = "${var.prefix}-web-server"
  description = "Image for web server built with packer from Mr.Prober images collection"
  uri = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/web-server/fd8cvg56al5ci85f4oun.qcow2"

  os_type = "LINUX"
  min_disk_size = 10
}

# Additional security group to targets attached together with instances-sg
resource "ycp_vpc_security_group" "targets" {
  folder_id = var.folder_id

  network_id = ycp_vpc_network.network.id
  name = "${var.prefix}-targets-sg"
  description = "Additional security group for targets"

  rule_spec {
    direction = "INGRESS"
    protocol_name = "TCP"
    description = "Allow 80/tcp by IPv4 and IPv6"
    cidr_blocks {
      v4_cidr_blocks = values(var.monitoring_network_ipv4_cidrs)
      v6_cidr_blocks = values(var.monitoring_network_ipv6_cidrs)
    }
    ports {
      from_port = 80
      to_port = 80
    }
  }
}

locals {
  _target_vms_list = flatten([
    for zone_id in var.zones : [
      for instance_index in range(var.target_count_per_zone) : {
        zone_id        = zone_id
        instance_index = instance_index + 1
        fqdn           = "target-${zone_id}-${instance_index + 1}.${var.prefix}.${var.dns_zone}"
        # IPv4 addresses for targets are 10.(1|2|3).250.(250|251|...)
        ipv4_address   = cidrhost(lookup(var.monitoring_network_ipv4_cidrs, zone_id), 250 * 256 + 250 + instance_index)
        ipv6_address   = cidrhost(lookup(var.monitoring_network_ipv6_cidrs, zone_id), 250 * 256 + 250 + instance_index)
      }
    ]
  ])

  target_vms = {
    for vm in local._target_vms_list: "${vm.zone_id}:${vm.instance_index}" => vm
  }
}

resource "ycp_microcosm_instance_group_instance_group" "targets" {
  folder_id = var.folder_id

  name = "${var.prefix}-targets"
  description = "Additional target instances"

  service_account_id = var.mr_prober_sa_id

  allocation_policy {
    dynamic "zone" {
      for_each = toset(var.zones)
      content {
        zone_id = zone.value
      }
    }
  }

  deploy_policy {
    max_unavailable = 1
    max_creating = length(var.zones)
    max_expansion = 0
    max_deleting = 1
  }

  scale_policy {
    fixed_scale {
      size = length(var.zones) * var.target_count_per_zone
    }
  }

  instance_template {
    name = "${var.prefix}-target-{instance.zone_id}-{instance.index_in_zone}"
    hostname = "{fqdn_{instance.zone_id}_{instance.index_in_zone}}"
    fqdn = "{fqdn_{instance.zone_id}_{instance.index_in_zone}}"
    description = "Target instance"

    service_account_id = var.mr_prober_sa_id

    platform_id = var.target_platform_id

    resources {
      memory = 2
      cores = 2
      core_fraction = 20
    }

    boot_disk {
      disk_spec {
        image_id = ycp_compute_image.web_server.id
        size = 10
      }
    }

    network_interface {
      subnet_ids = [for subnet in ycp_vpc_subnet.subnets: subnet.id]
      security_group_ids = [
        ycp_vpc_security_group.instances.id,
        ycp_vpc_security_group.targets.id
      ]
      primary_v4_address {
        address = "{ipv4_address_{instance.zone_id}_{instance.index_in_zone}}"
        dns_record_spec {
          fqdn = "target-{instance.zone_id}-{instance.index_in_zone}-v4.ru-central1.internal."
          ptr = true
        }
      }
      primary_v6_address {
        address = "{ipv6_address_{instance.zone_id}_{instance.index_in_zone}}"
        dns_record_spec {
          fqdn = "target-{instance.zone_id}-{instance.index_in_zone}-v6.ru-central1.internal."
          ptr = true
        }
      }
    }

    network_interface {
      subnet_ids = values(var.control_network_subnet_ids)
      primary_v6_address {
        dns_record_spec {
          dns_zone_id = var.dns_zone_id
          fqdn = "{fqdn_{instance.zone_id}_{instance.index_in_zone}}."
          ptr = true
        }
      }
    }

    labels = {
      abc_svc = "ycvpc"
      layer = "iaas"
      env = var.label_environment
    }

    metadata = {
      user-data = templatefile(
        "${path.module}/cloud-init.yaml",
        {
          hostname = "{fqdn_{instance.zone_id}_{instance.index_in_zone}}",
          stand_name = local.mr_prober_environment,
          cluster_id = var.cluster_id,
          agent_additional_metric_labels = var.agent_additional_metric_labels,
          s3_endpoint = var.s3_endpoint,
          runcmd = [],
          bootcmd = []
        }
      )
      skm = local.skm_metadata

      # See https://wiki.yandex-team.ru/cloud/devel/instance-group/internal/
      # Skm keys are regenerated on Creator restart, so ignore them.
      # If you want update secrets for agents, add some new metadata key (i.e. "secrets_versions = 1.0")
      internal-metadata-live-update-keys = "internal-metadata-live-update-keys,skm"
    }
  }

  dynamic "variable" {
    for_each = local.target_vms
    content {
      key = "ipv4_address_${variable.value.zone_id}_${variable.value.instance_index}"
      value = variable.value.ipv4_address
    }
  }

  dynamic "variable" {
    for_each = local.target_vms
    content {
      key = "ipv6_address_${variable.value.zone_id}_${variable.value.instance_index}"
      value = variable.value.ipv6_address
    }
  }

  dynamic "variable" {
    for_each = local.target_vms
    content {
      key = "fqdn_${variable.value.zone_id}_${variable.value.instance_index}"
      value = variable.value.fqdn
    }
  }
}

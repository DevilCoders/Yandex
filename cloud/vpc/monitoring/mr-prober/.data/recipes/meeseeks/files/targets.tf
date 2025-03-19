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

resource "ycp_compute_instance" "target" {
  for_each = local.target_vms

  folder_id = var.folder_id

  name = "${var.prefix}-target-${each.value.zone_id}-${each.value.instance_index}"
  hostname = each.value.fqdn
  fqdn = each.value.fqdn
  description = "Additional target instance in ${each.value.zone_id}"

  platform_id = var.target_platform_id
  zone_id = each.value.zone_id

  resources {
    memory = 2
    cores = 2
    core_fraction = 20
  }

  boot_disk {
    disk_spec {
      image_id = ycp_compute_image.web_server.id
    }
  }

  # Just a placeholder. See https://st.yandex-team.ru/CLOUD-97056
  gpu_settings {}

  network_interface {
    subnet_id = lookup(lookup(ycp_vpc_subnet.subnets, each.value.zone_id), "id")
    security_group_ids = [
      ycp_vpc_security_group.instances.id,
      ycp_vpc_security_group.targets.id
    ]
    primary_v4_address {
      address = each.value.ipv4_address
      dns_record {
        fqdn = "target-${each.value.zone_id}-${each.value.instance_index}-v4.ru-central1.internal."
        ptr = true
      }
    }
    primary_v6_address {
      address = each.value.ipv6_address
      dns_record {
        fqdn = "target-${each.value.zone_id}-${each.value.instance_index}-v6.ru-central1.internal."
        ptr = true
      }
    }
  }

  network_interface {
    subnet_id = lookup(var.control_network_subnet_ids, each.value.zone_id)
    primary_v6_address {
      dns_record {
        dns_zone_id = var.dns_zone_id
        fqdn = "${each.value.fqdn}."
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
        hostname = each.value.fqdn,
        stand_name = local.mr_prober_environment,
        cluster_id = var.cluster_id,
        agent_additional_metric_labels = var.agent_additional_metric_labels,
        s3_endpoint = var.s3_endpoint,
        runcmd = [],
        bootcmd = []
      }
    )
    skm = local.skm_metadata
  }

  service_account_id = var.mr_prober_sa_id

  allow_stopping_for_update = true

  # Due to bug (?) this field is marked as updated each time
  # See https://st.yandex-team.ru/CLOUD-91252#6207cce8f756102386d9107d
  lifecycle {
    ignore_changes = [
      local_disk
    ]
  }
}
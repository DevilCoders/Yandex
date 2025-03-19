locals {
  target_zone = "${element(var.zones, 0)}"
  host_suffix = "${lookup(var.host_suffix_for_zone, local.target_zone)}"

  metadata_module = {
    infra-configs = "${var.infra-configs}"
    docker-config = "${var.docker-config}"
    ssh-keys      = "${var.ssh-keys}"
    osquery_tag   = "${var.osquery_tag}"
  }

  metadata_full = "${merge(local.metadata_module, var.metadata)}"

  labels_module = {
    role                 = "${var.role_name}"
    cluster_id           = "${random_string.cluster-token.result}"
    skip_update_ssh_keys = "${var.skip_update_ssh_keys}"
  }

  labels_full = "${merge(local.labels_module, var.labels)}"

  # Hack: use length of num_interfaces in order to enable network_interface block.
  num_interfaces = "${var.underlay ? [] : [1]}"
  num_underlay_networks = "${var.underlay ? [1] : []}"
  num_secondary_disks = "${var.need_secondary_disk ? [1] : []}"
}

resource "ycp_compute_instance" "node" {
  count = "${var.instance_group_size}"

  name        = "${var.name_prefix}-${lookup(var.host_suffix_for_zone, element(var.zones, count.index))}-${floor(count.index / length(var.zones)) + count.index % length(var.zones) - index(var.zones, element(var.zones, count.index)) + 1}"
  fqdn        = "${var.hostname_prefix}-${lookup(var.host_suffix_for_zone, element(var.zones, count.index))}-${floor(count.index / length(var.zones)) + count.index % length(var.zones) - index(var.zones, element(var.zones, count.index)) + 1}.${var.hostname_suffix}"
  description = "${var.instance_description}"

  platform_id     = "${var.instance_platform_id}"
  pci_topology_id = "${var.instance_pci_topology_id}"

  resources {
    cores         = "${var.cores_per_instance}"
    memory        = "${var.memory_per_instance}"
    core_fraction = "${var.core_fraction_per_instance}"
  }

  boot_disk {
    disk_spec {
      image_id = "${var.image_id}"
      size     = "${var.disk_per_instance}"
      type_id  = "${var.disk_type}"
    }
  }

  dynamic "secondary_disk" {
    for_each = "${local.num_secondary_disks}"
    content {
      device_name = "${var.secondary_disk_device_name}"
      auto_delete = "${var.secondary_disk_auto_delete}"
      disk_id     = "${element(var.secondary_disks, count.index).id}"
    }
  }

  zone_id = "${element(var.zones, count.index)}"

  dynamic "underlay_network" {
    for_each = "${local.num_underlay_networks}"
    content {
      network_name = "underlay-v6"
    }
  }

  dynamic "network_interface" {
    for_each = "${local.num_interfaces}"
    content {
      subnet_id = "${lookup(var.subnets, element(var.zones, count.index))}"
      primary_v4_address {
        address = "${element(var.ipv4_addresses, count.index)}"
      }
      primary_v6_address {
        address = "${element(var.ipv6_addresses, count.index)}"
      }
      security_group_ids = "${var.security_group_ids}"
    }
  }

  metadata = "${merge(
    local.metadata_full,
    map("configs", "${element(var.configs, count.index)}"),
    map("podmanifest", "${element(var.podmanifest, count.index)}")
  )}"

  labels = "${merge(local.labels_full, map("node_id", count.index))}"

  service_account_id = "${var.service_account_id}"

  placement_policy {
    host_group = "${var.host_group}"
  }

  allow_stopping_for_update = true
}

resource "random_string" "cluster-token" {
  length  = 16
  special = false
  upper   = false
}

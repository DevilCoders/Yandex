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
}

resource "ycunderlay_compute_instance" "node" {
  count = "${var.instance_group_size}"

  name        = "${var.name_prefix}-${lookup(var.host_suffix_for_zone, element(var.zones, count.index))}-${floor(count.index / length(var.zones)) + count.index % length(var.zones) - index(var.zones, element(var.zones, count.index)) + 1}"
  fqdn        = "${var.hostname_prefix}-${lookup(var.host_suffix_for_zone, element(var.zones, count.index))}-${floor(count.index / length(var.zones)) + count.index % length(var.zones) - index(var.zones, element(var.zones, count.index)) + 1}.${var.hostname_suffix}"
  description = "${var.instance_description}"

  platform_id = "${var.instance_platform_id}"

  resources {
    cores  = "${var.cores_per_instance}"
    memory = "${var.memory_per_instance}"
    core_fraction = "${var.core_fraction_per_instance}"
  }

  boot_disk {
    initialize_params {
      image_id = "${var.image_id}"
      size     = "${var.disk_per_instance}"
      type     = "${var.disk_type}"
    }
  }

  zone = "${element(var.zones, count.index)}"

  underlay_network = "${var.underlay ? "underlay-v6" : ""}"

  dynamic "network_interface" {
    for_each = "${local.num_interfaces}"
    content {
      subnet_id    = "${lookup(var.subnets, element(var.zones, count.index))}"
      ipv6         = "${var.ipv6}"
      ip_address   = "${element(var.ipv4_addresses, count.index)}"
      ipv6_address = "${element(var.ipv6_addresses, count.index)}"
    }
  }

  metadata = "${merge(
    local.metadata_full,
    map("configs", "${element(var.configs, count.index)}"),
    map("podmanifest", "${element(var.podmanifest, count.index)}")
  )}"

  labels = "${merge(local.labels_full, map("node_id", count.index))}"

  service_account_id = "${var.service_account_id}"

  host_group = "${var.host_group}"
}

resource "random_string" "cluster-token" {
  length  = 16
  special = false
  upper   = false
}

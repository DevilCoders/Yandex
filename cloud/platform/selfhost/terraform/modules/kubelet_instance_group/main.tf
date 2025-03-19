locals {
  target_zone = "${element(var.zones, 0)}"
  host_suffix = "${lookup(var.host_suffix_for_zone, local.target_zone)}"

  metadata_module = {
    configs       = "${var.configs}"
    infra-configs = "${var.infra-configs}"
    podmanifest   = "${var.podmanifest}"
    docker-config = "${var.docker-config}"
    ssh-keys      = "${var.ssh-keys}"
  }

  metadata_full = "${merge(local.metadata_module, var.metadata)}"

  labels_module = {
    role                 = "${var.role_name}"
    cluster_id           = "${random_string.cluster-token.result}"
    skip_update_ssh_keys = "${var.skip_update_ssh_keys}"
  }

  labels_full = "${merge(local.labels_module, var.labels)}"
}

resource "yandex_compute_instance" "node" {
  count = "${var.instance_group_size}"

  name        = "${var.name_prefix}-${lookup(var.host_suffix_for_zone, element(var.zones, count.index))}-${count.index / length(var.zones) + count.index % length(var.zones) - index(var.zones, element(var.zones, count.index)) + 1}"
  hostname    = "${var.hostname_prefix}-${lookup(var.host_suffix_for_zone, element(var.zones, count.index))}-${count.index / length(var.zones) + count.index % length(var.zones) - index(var.zones, element(var.zones, count.index)) + 1}"
  description = "${var.instance_description}"

  resources {
    cores  = "${var.cores_per_instance}"
    memory = "${var.memory_per_instance}"
  }

  boot_disk {
    initialize_params {
      image_id = "${var.image_id}"
      size     = "${var.disk_per_instance}"
      type     = "${var.disk_type}"
    }
  }

  zone = "${element(var.zones, count.index)}"

  network_interface {
    subnet_id = "${lookup(var.subnets, element(var.zones, count.index))}"
    ipv6      = "${var.ipv6}"
  }

  metadata = "${local.metadata_full}"

  labels = "${merge(local.labels_full, map("node_id", count.index))}"
}

resource "random_string" "cluster-token" {
  length  = 16
  special = false
  upper   = false
}

variable "conductor_group" {
  default = ""
}

locals {
  target_zone = element(var.zone_name, 0)

  metadata_module = {
    infra-configs = var.infra-configs
    docker-config = var.docker-config
    ssh-keys      = var.ssh-keys
  }

  metadata_identical = merge(local.metadata_module, var.metadata)

  labels_module = {
    role                 = var.role_name
    cluster_id           = random_string.cluster-token.result
    skip_update_ssh_keys = var.skip_update_ssh_keys

    conductor-group = var.conductor_group
  }

  labels_full = merge(local.labels_module, var.labels)
}

data "template_file" "node-name" {
  count    = var.instance_group_size
  template = "$${name}"

  vars = {
    name = "${var.name_prefix}-${element(var.zone_suffix, count.index)}${format("%02d", floor(1 + (count.index / length(var.zone_suffix))))}"
  }
}

resource "yandex_compute_instance" "node" {
  count = var.instance_group_size

  name     = element(data.template_file.node-name.*.rendered, count.index)
  hostname = element(data.template_file.node-name.*.rendered, count.index)

  description = var.instance_description
  allow_stopping_for_update = true
  service_account_id  = var.instance_service_account_id

  platform_id = var.platform_id

  resources {
    cores         = var.cores_per_instance
    memory        = var.memory_per_instance
    core_fraction = var.core_fraction_per_instance
  }

  boot_disk {
    initialize_params {
      image_id = var.image_id
      size     = var.disk_per_instance
      type     = var.disk_type

    }
  }

  zone = element(var.zone_name, count.index)

  network_interface {
    subnet_id    = lookup(var.subnets, element(var.zone_name, count.index))
    ipv6         = var.ipv6
    ip_address   = element(var.ipv4_addrs, count.index)
    ipv6_address = element(var.ipv6_addrs, count.index)
  }

  metadata = merge(
    local.metadata_identical,
    length(var.metadata_per_instance) > 0 ? var.metadata_per_instance[count.index] : {},
    map("yandex-dns", "${element(data.template_file.node-name.*.rendered, count.index)}"),
    map("configs", "${element(var.configs, count.index)}"),
    map("podmanifest", "${element(var.podmanifest, count.index)}")
  )

  labels = merge(local.labels_full,
    map("node_id", count.index),
    map("yandex-dns", "${element(data.template_file.node-name.*.rendered, count.index)}"),
    map("conductor-dc", "${element(var.zone_suffix, count.index)}")
  )

  dynamic "secondary_disk" {
    for_each = length(var.secondary_disks) > 0 ? [var.secondary_disks[count.index]] : []

    content {
      disk_id = secondary_disk.value
    }
  }

  provisioner "local-exec" {
    when        = destroy
    command     = "${path.module}/../../common/juggler-downtime.sh on ${self.network_interface.0.ipv6_address}"
    environment = {}

    on_failure = continue
  }
}

resource "random_string" "cluster-token" {
  length  = 16
  special = false
  upper   = false
}

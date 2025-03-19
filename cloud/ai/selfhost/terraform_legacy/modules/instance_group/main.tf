locals {
  metadata_module = {
    configs       = var.configs
    podmanifest   = var.podmanifest
    docker-config = var.docker_config
  }

  metadata_optional_gpus = {
    exists = {
      internal-gpus = var.gpus_per_instance
    }
    not_exists = {}
  }

  metadata_full = merge(
    local.metadata_module,
    var.metadata,
    local.metadata_optional_gpus[var.gpus_per_instance != 0 ? "exists" : "not_exists"]
  )

  labels_module = {
    target_group_name    = var.target_group_name
    skip_update_ssh_keys = var.skip_update_ssh_keys
  }

  labels_full = merge(local.labels_module, var.labels)
}

data "null_data_source" "iterate_subnets" {
  count = length(var.zones)
  inputs = {
    value = lookup(var.subnets, var.zones[count.index])
  }
}

resource "yandex_compute_instance_group" "group" {
  name               = var.name
  description        = var.description
  folder_id          = var.folder_id
  service_account_id = var.service_account_id
  load_balancer {
    target_group_name        = var.target_group_name
    target_group_description = var.target_group_description
  }
  instance_template {
    platform_id = var.platform_id
    resources {
      memory = var.memory_per_instance
      cores  = var.cores_per_instance
    }
    boot_disk {
      mode = "READ_WRITE"
      initialize_params {
        image_id = var.image_id
        size     = var.disk_per_instance
        type     = var.disk_type
      }
    }
    network_interface {
      subnet_ids = data.null_data_source.iterate_subnets.*.outputs.value
      ipv6       = var.ipv6
    }
    description = var.instance_description
    labels = local.labels_full
    metadata = local.metadata_full
  }

  scale_policy {
    fixed_scale {
      size = var.instance_group_size
    }
  }

  allocation_policy {
    zones = var.zones
  }

  deploy_policy {
    max_unavailable     = var.max_unavailable
    max_creating        = var.max_creating
    max_expansion       = var.max_expansion
    max_deleting        = var.max_deleting
    startup_duration    = var.startup_duration
  }
}

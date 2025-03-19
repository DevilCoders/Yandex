locals {
  metadata_module = {
    configs       = var.configs
    podmanifest   = var.podmanifest
    docker-config = var.docker_config
  }

  metadata_full = {
    metadata = merge(local.metadata_module, var.metadata)
  }

  labels_module = {
    target_group_name    = var.target_group_name
    skip_update_ssh_keys = var.skip_update_ssh_keys
  }

  labels_full = {
    labels = merge(local.labels_module, var.labels)
  }

  yaml_config_env = {
    // Instance group parameters
    name                      = var.name
    description               = var.description
    service_account_id        = var.service_account_id

    instance_group_size = var.instance_group_size
    max_unavailable     = var.max_unavailable
    max_creating        = var.max_creating
    max_deleting        = var.max_deleting
    max_expansion       = var.max_expansion
    startup_duration    = var.startup_duration
    zones               = var.zones

    target_group_name        = var.target_group_name
    target_group_description = var.target_group_description
    health_checks_spec       = ""

    // Instance config
    platform_id         = var.platform_id
    memory_per_instance = var.memory_per_instance
    cores_per_instance  = var.cores_per_instance
    gpus_per_instance   = var.gpus_per_instance
    disk_type           = var.disk_type
    disk_per_instance   = var.disk_per_instance
    subnets             = data.null_data_source.iterate_subnets.*.outputs.value
    image_id            = var.image_id

    labels   = indent(2, yamlencode(local.labels_full))
    metadata = indent(2, yamlencode(local.metadata_full))
    secondary_disk_specs = indent(4, yamlencode(var.secondary_disk_specs))
    scheduling_policy = indent(4, yamlencode(var.scheduling_policy))
    instance_name = var.instance_name
    instance_hostname = var.instance_hostname
  }
}

data "null_data_source" "iterate_subnets" {
  count = length(var.zones)
  inputs = {
    value = lookup(var.subnets, var.zones[count.index])
  }
}


resource "local_file" "ig_yaml" {
  filename = "${path.root}/generated-ig.yml"
  content  = templatefile(
    "${path.module}/files/ig.tpl.yml",
    local.yaml_config_env,
  )
}

locals {
    metadata_module = {
        configs       = var.configs
        podmanifest   = var.podmanifest
        docker-config = var.docker_config
    }

    metadata_full = merge(local.metadata_module, var.metadata)

    labels_module = {
        target_group_name    = var.target_group_name
        skip_update_ssh_keys = var.skip_update_ssh_keys
    }

    labels_full = merge(local.labels_module, var.labels)
}

provider "ycp" {
    prod = true
}

data "null_data_source" "iterate_subnets" {
  count = length(var.zones)
  inputs = {
    value = lookup(var.subnets, var.zones[count.index])
  }
}

resource ycp_microcosm_instance_group_instance_group group {
    service_account_id = var.service_account_id

    name = var.name
    description = var.description

    folder_id = var.folder_id

    deploy_policy {
        max_creating = var.max_creating
        max_deleting = var.max_deleting
        max_expansion = var.max_expansion
        max_unavailable = var.max_unavailable
        startup_duration = var.startup_duration
    }

    platform_l7_load_balancer_spec {
        preferred_ip_version = "IPV6"
        target_group_spec {
            name = var.target_group_name
            description = var.target_group_description
        }
    }

    allocation_policy {
        dynamic zone {
            for_each = var.zones
            content {
                zone_id = zone.value
            }
        }
    }

    scale_policy {
        fixed_scale {
            size = var.instance_group_size
        }
    }

    instance_template {
        name = var.instance_name
        hostname = var.instance_hostname
        service_account_id = var.service_account_id
        platform_id = var.platform_id

        labels = local.labels_full

        resources {
            memory = var.memory_per_instance
            cores = var.cores_per_instance
            core_fraction = 100
            gpus = var.gpus_per_instance
        }

        boot_disk {
            mode = "READ_WRITE"
            disk_spec {
                type_id = var.disk_type
                size = var.disk_per_instance
                image_id = var.image_id
            }
        }

        dynamic secondary_disk {
            for_each = var.secondary_disk_specs
            content {
                device_name = secondary_disk.value.device_name
                mode = secondary_disk.value.mode
                disk_spec {
                  type_id = lookup(secondary_disk.value.disk_spec, "type_id", null)
                  image_id = lookup(secondary_disk.value.disk_spec, "image_id", null)
                  snapshot_id = lookup(secondary_disk.value.disk_spec, "snapshot_id", null)
                  size = lookup(secondary_disk.value.disk_spec, "size", null)
                  preserve_after_instance_delete = lookup(secondary_disk.value.disk_spec, "preserve_after_instance_delete", null)
                  description = lookup(secondary_disk.value.disk_spec, "description", null)
                }
            }
        }

        network_interface {
            subnet_ids = data.null_data_source.iterate_subnets.*.outputs.value
            primary_v4_address {}
            primary_v6_address {}
        }

        dynamic "network_interface" {
          for_each = var.additional_subnets
          content {
              subnet_ids = network_interface.value
              primary_v6_address {}
          }
        }

        scheduling_policy {
            termination_grace_period = var.scheduling_policy.termination_grace_period
        }

        //noinspection HCLSimplifyExpression
        metadata = local.metadata_full
    }
}

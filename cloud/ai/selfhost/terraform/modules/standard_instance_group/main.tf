/*
  standard instance group based on ycp_instance_group
  used internaly in standard service
 */

locals {
  unique_instance_name = "${var.name}-{instance.index}"

  metadata_module = {
    configs        = var.configs
    podmanifest    = var.podmanifest
    docker-config  = var.docker_config
    user-data      = var.user_data
    skm            = var.secrets_bundle
    instance_index = "{instance.index}"
    instance_zone  = "{instance.zone_id}"

    // This metadata required by startup systemd daemon
    // that initializes hostname and fqdn accordingly
    // Get rid of this script and cloud-init as single initialization daemon
    shortname = "${local.unique_instance_name}"
    nsdomain  = var.environment == "preprod" ? "datasphere.cloud-preprod.yandex.net" : "datasphere.cloud.yandex.net"
  }

  metadata_full = merge(local.metadata_module, var.additional_metadata)

  labels_module = {
    env = var.environment

    // Probably not usable
    // target_group_name    = var.target_group_name

    // Always skipping ssh keys updating process
    // https://a.yandex-team.ru/arc/trunk/arcadia/cloud/tools/update-ssh-keys/main.py?#L200
    skip_update_ssh_keys = "true"

    // Labels required by YTR to sync with conductor infrastructure
    abc_svc    = "ycai"
    yandex-dns = "ig"

    conductor-group = var.environment == "preprod" ? "yc_ai_preprod" : "yc_ai_prod"

    // TODO: Add description for thos labels
    layer = "saas"
  }

  labels_full = merge(local.labels_module, var.additional_labels)
}

resource "ycp_microcosm_instance_group_instance_group" "instance_group" {
  // Secrets passthough
  folder_id          = var.folder_id
  service_account_id = var.service_account_id

  // Instance group description
  name        = var.name
  description = var.description

  deploy_policy {
    max_creating     = var.deploy_policy.max_creating
    max_deleting     = var.deploy_policy.max_deleting
    max_expansion    = var.deploy_policy.max_expansion
    max_unavailable  = var.deploy_policy.max_unavailable
    startup_duration = var.deploy_policy.startup_duration
  }

  allocation_policy {
    dynamic "zone" {
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

  # dynamic "load_balancer_spec" {
  #   for_each = var.nlb_target ? [true] : []
  #   content {
  #     target_group_spec {
  #       name = "nlb-tg-${var.name}"
  #     }
  #   }
  # }

  instance_template {
    name               = local.unique_instance_name
    hostname           = local.unique_instance_name
    service_account_id = var.service_account_id
    platform_id        = var.platform_id

    resources {
      core_fraction = 100
      cores         = var.resources_per_instance.cores
      gpus          = var.resources_per_instance.gpus
      memory        = var.resources_per_instance.memory
    }

    boot_disk {
      mode = "READ_WRITE"
      disk_spec {
        type_id  = var.boot_disk_spec.type
        size     = var.boot_disk_spec.size
        image_id = var.boot_disk_spec.image_id
      }
    }

    dynamic "secondary_disk" {
      for_each = var.secondary_disk_specs
      content {
        device_name = secondary_disk.value.device_name
        mode        = secondary_disk.value.mode
        disk_spec {
          type_id                        = lookup(secondary_disk.value.disk_spec, "type_id", null)
          image_id                       = lookup(secondary_disk.value.disk_spec, "image_id", null)
          snapshot_id                    = lookup(secondary_disk.value.disk_spec, "snapshot_id", null)
          size                           = lookup(secondary_disk.value.disk_spec, "size", null)
          preserve_after_instance_delete = lookup(secondary_disk.value.disk_spec, "preserve_after_instance_delete", null)
          description                    = lookup(secondary_disk.value.disk_spec, "description", null)
        }
      }
    }

    dynamic "network_interface" {
      for_each = var.networks
      content {
        network_id = network_interface.value.interface.network
        subnet_ids = [
          for zone, subnet_id in network_interface.value.interface.subnets :
          subnet_id
          if contains(var.zones, zone)
        ]

        dynamic "primary_v4_address" {
          for_each = network_interface.value.interface.ipv4 ? [true] : []
          content {}
        }

        dynamic "primary_v6_address" {
          for_each = network_interface.value.interface.ipv6 ? [true] : []
          content {
            dynamic "dns_record_spec" {
              for_each = network_interface.value.dns
              content {
                fqdn        = dns_record_spec.value.fqdn
                dns_zone_id = dns_record_spec.value.dns_zone_id
              }
            }
          }
        }
      }
    }

    scheduling_policy {
      termination_grace_period = var.scheduling_policy.termination_grace_period
    }

    metadata = local.metadata_full
    labels   = local.labels_full
  }

  # application_load_balancer_spec {
  #   target_group_spec {
  #     name        = var.name
  #     description = var.description
  #   }
  #   max_opening_traffic_duration = "600s" # 10 minutes
  # }

  // TODO: This is deprecated field application_load_balancer_spec
  //       should be used instead, but there is no migration for it now
  platform_l7_load_balancer_spec {
    preferred_ip_version = "IPV6"
    target_group_spec {
      name        = var.target_group_name == "" ? var.name : var.target_group_name
      description = var.target_group_description == "" ? var.description : var.target_group_description
    }
  }
}

resource ycp_microcosm_instance_group_instance_group group {
  name        = var.name
  description = var.description

  folder_id          = var.folder_id
  service_account_id = var.service_account_id

  scale_policy {
    fixed_scale {
      size = var.amount
    }
  }

  allocation_policy {
    dynamic zone {
      for_each = keys(var.subnets)
      content {
        zone_id = zone.value
      }
    }
  }

  deploy_policy {
    max_deleting    = 1
    max_unavailable = 1
    max_creating    = var.amount
    max_expansion   = 0
  }

  labels = {
    environment = local.environment
    service     = var.service
  }

  dynamic load_balancer_spec {
    for_each = local.l3_req_list
    content {
      target_group_spec {
        name          = local.l3_tg_name
        labels        = {
          layer   = "paas"
          abc_svc = var.abc_service_name
        }
        address_names = [
          "ipv4",
          "ipv6",
        ]
      }
    }
  }

  dynamic platform_l7_load_balancer_spec {
    for_each = local.l7_req_list
    content {
      preferred_ip_version = "IP_VERSION_UNSPECIFIED"
      target_group_spec {
        name          = local.l7_tg_name
        labels        = {
          layer   = "paas"
          abc_svc = var.abc_service_name
        }
        address_names = [
          "ipv6",
        ]
      }
    }
  }

  instance_template {
    name     = local.dns_prefix
    hostname = local.dns_prefix
    fqdn     = local.dns_prefix

    service_account_id = var.instance_service_account_id
    platform_id        = var.platform_id

    resources {
      memory        = var.memory
      cores         = var.cores
      core_fraction = 100
    }

    scheduling_policy {
      preemptible = false
    }

    boot_disk {
      device_name = "boot"
      mode        = "READ_WRITE"
      disk_spec {
        size     = var.boot_disk_size
        image_id = var.boot_disk_image_id
      }
    }
    secondary_disk {
      device_name = "log"
      mode        = "READ_WRITE"
      disk_spec {
        size    = var.secondary_disk_size
        type_id = "network-hdd"
      }
    }

    network_interface {
      subnet_ids = values(var.subnets)
      primary_v4_address {
        name = "ipv4"
      }
      primary_v6_address {
        name = "ipv6"
      }
    }

    labels = {
      environment     = local.environment
      env             = "prod"
      layer           = "paas"
      abc_svc         = var.abc_service_name
      conductor-group = var.conductor_group
      conductor-role  = var.service
      conductor-dc    = local.template_internal_dc
      yandex-dns      = local.dns_prefix
    }

    metadata = merge({
      yandex-dns  = local.dns_prefix
      shortname   = local.shortname
      nsdomain    = local.nsdomain
      ssh-keys    = var.ssh-keys
      decode_key  = var.decode_key
      internal_dc = local.template_internal_dc
      user-data   = file("${path.module}/config/cloud-init.yaml")
    }, var.instance_metadata)


  }
}

data yandex_compute_instance_group group_data {
  instance_group_id = ycp_microcosm_instance_group_instance_group.group.id
}
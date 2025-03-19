resource yandex_compute_instance_group group {
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
    zones = keys(var.subnets)
  }

  deploy_policy {
    max_deleting    = var.amount > 2 ? floor((var.amount - 1)/2) : max(var.amount - 1, 1)
    max_unavailable = var.amount > 2 ? floor((var.amount - 1)/2) : max(var.amount - 1, 1)
    max_creating    = var.amount
    max_expansion   = 0
  }

  labels = {
    environment = local.environment
    service     = var.service
  }

  dynamic load_balancer {
    for_each = local.lb_req_list
    content {
      target_group_name   = local.lb_tg_name
      target_group_labels = {
        layer   = "paas"
        abc_svc = var.abc_service_name
      }
    }
  }

  instance_template {
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
      initialize_params {
        image_id = var.boot_disk_image_id
        size     = var.boot_disk_size
      }
    }
    secondary_disk {
      device_name = "log"
      mode        = "READ_WRITE"
      initialize_params {
        size = var.secondary_disk_size
      }
    }

    network_interface {
      subnet_ids = values(var.subnets)
      ipv6       = true
    }

    labels = {
      environment     = local.environment
      env             = "pre-prod"
      layer           = "paas"
      abc_svc         = var.abc_service_name
      conductor-group = var.conductor_group
      conductor-role  = var.service
      conductor-dc    = local.template_internal_dc
      yandex-dns      = local.dns_prefix
    }

    metadata = merge({
      internal-name     = local.dns_prefix
      internal-hostname = local.dns_prefix
      internal-fqdn     = local.dns_prefix
      yandex-dns        = local.dns_prefix
      ssh-keys          = var.ssh-keys
      decode_key        = var.decode_key
      internal_dc       = local.template_internal_dc
      user-data         = file("${path.module}/config/cloud-init.yaml")
    }, var.instance_metadata)
  }
}

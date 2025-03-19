data "yandex_compute_image" "container-optimized-image" {
  family = "container-optimized-image"
}

data "yandex_dns_zone" "oncall" {
  dns_zone_id = var.dns_zone_id
}

resource "ycp_microcosm_instance_group_instance_group" "dutybot" {
  folder_id   = var.folder_id

  name        = "dutybot-${var.environment}"
  description = "Dutybot instance group"

  service_account_id = var.service_account_id

  instance_template {
    resources {
      memory = 4
      cores = 2
      core_fraction = 20
    }
    
    boot_disk {
      disk_spec {
        size = 30
        type_id  = "network-ssd"
        image_id = data.yandex_compute_image.container-optimized-image.id
      }
    }

    network_interface {
      subnet_ids = values(var.subnet_ids)
      primary_v4_address {
        name = "v4"
      }
      primary_v6_address {
        name = "v6"
        dns_record_spec {
          fqdn = "bot-${var.environment}-{instance.internal_dc}{instance.index_in_zone}.${data.yandex_dns_zone.oncall.zone}"
          dns_zone_id = data.yandex_dns_zone.oncall.id
          ptr = true
        }
        dns_record_spec {
          fqdn = "bot-${var.environment}-backends.${data.yandex_dns_zone.oncall.zone}"
          dns_zone_id = data.yandex_dns_zone.oncall.id
        }
      }
    }

    name = "bot-${var.environment}-{instance.internal_dc}{instance.index_in_zone}"
    description = "Dutybot backend"

    metadata = {
      docker-compose = templatefile("${path.module}/docker-compose.yaml", {
        version                      = var.docker_image_version,
        environment                  = var.environment,
        telegram_webhook_url         = "https://bot-telegram-${var.environment}.${trimsuffix(data.yandex_dns_zone.oncall.zone, ".")}/bot${var.bot_id}",
        yandex_messenger_webhook_url = "https://bot-yandex-messenger-${var.environment}.${trimsuffix(data.yandex_dns_zone.oncall.zone, ".")}/yandex"
        run_yandex_messenger_bot     = var.run_yandex_messenger_bot
      })
      user-data = file("${path.module}/cloud_config.yaml")
      enable-oslogin = true
    }

    labels = {
      layer = "paas"
      abc_svc = "cloud_oncall"
      env = var.environment
    }

    platform_id = "standard-v2"

    service_account_id = var.service_account_id
  }

  allocation_policy {
    zone { zone_id = "ru-central1-a" }
    zone { zone_id = "ru-central1-b" }
    zone { zone_id = "ru-central1-c" }
  }

  deploy_policy {
    max_unavailable = 2
    max_creating = 3
    max_expansion = 1
    max_deleting = 1
  }

  scale_policy {
    fixed_scale {
      size = 3
    }
  }

  application_load_balancer_spec {
    target_group_spec {
      name = "dutybot-${var.environment}-target-group"
      description = "Target group for Load balancer for Dutybot [${var.environment}]"
      address_names = ["v4"]
    }
  }

  ### Now HCaaS is incompatible with ALB,
  ### if healthchecks are running on the same port as ALB

  # health_checks_spec {
  #   health_check_spec {
  #     address_names = ["v4"]
  #     interval = "20s"
  #     timeout = "5s"
  #     healthy_threshold = 2
  #     unhealthy_threshold = 5
  #     http_options {
  #       port = 8080
  #       path = "/"
  #     }
  #   }
  # }
}




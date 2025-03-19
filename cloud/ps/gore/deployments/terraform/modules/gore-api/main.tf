data "yandex_compute_image" "container-optimized-image" {
  family = "container-optimized-image"
}

data "yandex_dns_zone" "oncall" {
  dns_zone_id = var.dns_zone_id
}

locals {
  config_path = "/var/run/gore"

  db_host = join(",", yandex_mdb_mongodb_cluster.gore-mongodb.host.*.name)
  db_user = one([for user in yandex_mdb_mongodb_cluster.gore-mongodb.user : user if user.name == "gore"])
}

resource "random_password" "tvm_token" {
  length  = 32
  special = false
  upper   = false
  lower   = true
  number  = true
}

resource "ycp_microcosm_instance_group_instance_group" "gore" {
  folder_id = var.folder_id

  name        = "gore-${var.environment}"
  description = "Gore-API instance group"

  service_account_id = var.service_account_id

  instance_template {
    resources {
      memory        = 4
      cores         = 2
      core_fraction = 20
    }

    boot_disk {
      disk_spec {
        size     = 30
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
          fqdn        = "gore-${var.environment}-{instance.internal_dc}{instance.index_in_zone}.${data.yandex_dns_zone.oncall.zone}"
          dns_zone_id = data.yandex_dns_zone.oncall.id
          ptr         = true
        }
        dns_record_spec {
          fqdn        = "gore-${var.environment}-backends.${data.yandex_dns_zone.oncall.zone}"
          dns_zone_id = data.yandex_dns_zone.oncall.id
        }
      }
    }

    name        = "gore-${var.environment}-{instance.internal_dc}{instance.index_in_zone}"
    description = "GORE backend"

    metadata = {
      docker-compose = templatefile("${path.module}/docker-compose.yaml", {
        version          = var.docker_image_version,
        environment      = var.environment,
        tvm_token        = random_password.tvm_token.result,
        mongo_user       = local.db_user.name,
        mongo_password   = local.db_user.password,
        mongo_host       = local.db_host,
        gore_config_path = "${local.config_path}/gore-config.json"
        tvm_config_path  = "${local.config_path}/tvm-config.json"
      })
      user-data = templatefile(
        "${path.module}/cloud_config.yaml",
        {
          gore_config      = file("${path.module}/../../../../configs/config.json")
          gore_config_path = "${local.config_path}/gore-config.json"
          tvm_secret       = "${var.tvm_secret}"
          tvm_config_path  = "${local.config_path}/tvm-config.json"
        }
      )
      enable-oslogin = true
    }

    labels = {
      layer   = "paas"
      abc_svc = "cloud_oncall"
      env     = var.environment
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
    max_creating    = 3
    max_expansion   = 1
    max_deleting    = 1
  }

  scale_policy {
    fixed_scale {
      size = 3
    }
  }

  application_load_balancer_spec {
    target_group_spec {
      name          = "gore-${var.environment}-target-group"
      description   = "Target group for Load balancer for Gore-API [${var.environment}]"
      address_names = ["v6"]
    }
  }

  health_checks_spec {
    health_check_spec {
      address_names       = ["v4"]
      interval            = "20s"
      timeout             = "5s"
      healthy_threshold   = 2
      unhealthy_threshold = 5
      http_options {
        port = 8080
        path = "/ping"
      }
    }
  }
}

resource "ycp_vpc_address" "this" {
  lifecycle {
    prevent_destroy = true
  }

  name = "gore-${var.environment}-l7-ipv6"
  folder_id = var.folder_id

  ipv6_address_spec {
    requirements {
      hints = ["yandex-only"]
    }
  }

  reserved = true
}

resource "yandex_dns_recordset" "gore-preprod" {
  count   = var.need_dns_record ? 1 : 0
  zone_id = data.yandex_dns_zone.oncall.id
  name    = "resps-api-preprod.${data.yandex_dns_zone.oncall.zone}"
  type    = "AAAA"
  ttl     = 600
  data    = [ycp_vpc_address.this.ipv6_address[0].address]
}

resource "ycp_dns_dns_record_set" "aaaa" {
  zone_id = data.yandex_dns_zone.oncall.id
  name    = "lb-gore-${var.environment}.${data.yandex_dns_zone.oncall.zone}"
  type    = "AAAA"
  ttl     = 600
  data    = [ycp_vpc_address.this.ipv6_address[0].address]
}

# See https://clubs.at.yandex-team.ru/ycp/4189
# and https://storage.cloud-preprod.yandex.net/terraform/docs/latest/resources/certificatemanager_certificate_request.html
resource "ycp_certificatemanager_certificate_request" "resps" {
  description = "Certificate for ${var.api_domain}"
  name = "resps-${var.environment}-certificate"

  domains = [
    var.api_domain
  ]

  folder_id = var.folder_id

  cert_provider = "INTERNAL_CA"

  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}

resource "ycp_platform_alb_load_balancer" "this" {
  name = "gore-${var.environment}-l7"
  folder_id = var.folder_id

  listener {
    name = "tls"

    endpoint {
      ports = [443]

      address {
        external_ipv6_address {
          address = ycp_vpc_address.this.ipv6_address[0].address
          yandex_only = true
        }
      }
    }

    tls {
      default_handler {
        certificate_ids = [ycp_certificatemanager_certificate_request.resps.id]

        http_handler {
          allow_http10   = false
          http_router_id = ycp_platform_alb_http_router.this.id
          is_edge = false

          http2_options {
            initial_stream_window_size     = 1024 * 1024
            initial_connection_window_size = 1024 * 1024
            max_concurrent_streams = 100
          }
        }
      }
      tcp_options {
        connection_buffer_limit_bytes = 32768
      }
    }
  }

  region_id = "ru-central1"
  network_id = var.network_id

  allocation_policy {
    location {
      subnet_id = var.subnet_ids["ru-central1-a"]
      zone_id   = "ru-central1-a"

      disable_traffic = false
    }

    location {
      subnet_id = var.subnet_ids["ru-central1-b"]
      zone_id   = "ru-central1-b"

      disable_traffic = false
    }

    location {
      subnet_id = var.subnet_ids["ru-central1-c"]
      zone_id   = "ru-central1-c"

      disable_traffic = false
    }
  }

  depends_on = [
    ycp_microcosm_instance_group_instance_group.gore,
  ]
}

resource "ycp_platform_alb_http_router" "this" {
  name = "gore-${var.environment}-l7"
  folder_id = var.folder_id
}

resource "ycp_platform_alb_virtual_host" "this" {
  name = "default"

  authority = [var.api_domain]
  http_router_id = ycp_platform_alb_http_router.this.id

  route {
    name = "default"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = ycp_platform_alb_backend_group.this.id
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "this" {
  name = "gore-${var.environment}-l7"
  folder_id = var.folder_id

  http {
    backend {
      name = "gore"

      port = 8080
      target_group {
        target_group_id = ycp_microcosm_instance_group_instance_group.gore.application_load_balancer_state[0].target_group_id
      }

      healthchecks {
        timeout             = "5s"
        interval            = "20s"
        healthy_threshold   = 2
        unhealthy_threshold = 2
        http {
          path = "/ping"
        }
      }
    }
  }
}

resource "yandex_mdb_mongodb_cluster" "gore-mongodb" {
  name        = "gore-${var.environment}"
  description = "Services, shifts and templates are here"
  environment = "PRODUCTION"
  network_id  = var.network_id
  deletion_protection = true

  cluster_config {
    version = "4.4"
  }

  labels = {
    env     = "prod"
    part_of = "gore-api"
    abc_svc = "cloud_oncall"
  }

  database {
    name = "gore"
  }

  user {
    name     = "gore"
    password = var.mongo_users["gore"]
    permission {
      database_name = "gore"
      roles         = ["readWrite"]
    }
  }

  user {
    name     = "andgein"
    password = var.mongo_users["andgein"]
    permission {
      database_name = "gore"
      roles         = ["readWrite"]
    }
  }

  resources {
    resource_preset_id = "s2.small"
    disk_size          = 100
    disk_type_id       = "local-ssd"
  }

  host {
    zone_id          = "ru-central1-a"
    subnet_id        = var.subnet_ids["ru-central1-a"]
    assign_public_ip = true
  }

  host {
    zone_id          = "ru-central1-b"
    subnet_id        = var.subnet_ids["ru-central1-b"]
    assign_public_ip = true
  }

  host {
    zone_id          = "ru-central1-c"
    subnet_id        = var.subnet_ids["ru-central1-c"]
    assign_public_ip = true
  }

  maintenance_window {
    type = "ANYTIME"
  }
}

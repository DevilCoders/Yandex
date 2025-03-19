locals {
  telegram_messenger = {
    ip_version = "ipv4",
    port = 8080,
    domain = trimsuffix("bot-telegram-${var.environment}.${data.yandex_dns_zone.oncall.zone}", ".")
  }

  yandex_messenger = {
    ip_version = "ipv6",
    port = 8081,
    domain = trimsuffix("bot-yandex-messenger-${var.environment}.${data.yandex_dns_zone.oncall.zone}", ".")
  }

  messengers = var.run_yandex_messenger_bot ? {
    telegram = local.telegram_messenger, 
    yandex-messenger = local.yandex_messenger
  } : {
    telegram = local.telegram_messenger
  }
}

# Certificates 

resource "ycp_certificatemanager_certificate_request" "dutybot" {
  for_each = local.messengers

  description = "Certificate for ${each.value.domain}"
  name = "dutybot-${each.key}-${var.environment}-certificate"

  domains = [
    each.value.domain
  ]

  folder_id = var.folder_id

  cert_provider = "INTERNAL_CA"

  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}

# IP Addresses

resource "ycp_vpc_address" "balancer" {
  for_each = local.messengers

  lifecycle {
    prevent_destroy = true
  }

  name      = "dutybot-${each.key}-${var.environment}"
  folder_id = var.folder_id

  dynamic "external_ipv4_address_spec" {
    for_each = each.value.ip_version == "ipv4" ? ["ipv4"] : []
    content {
      region_id = "ru-central1"
    }
  }

  dynamic "ipv6_address_spec" {
    for_each = each.value.ip_version == "ipv6" ? ["ipv6"] : []
    content {
      requirements {
        hints = ["yandex-only"]
      }
    }
  }

  reserved = true
}

# DNS Records

resource "yandex_dns_recordset" "balancer" {
  for_each = local.messengers

  lifecycle {
    prevent_destroy = true
  }

  name = "${each.value.domain}."
  zone_id = data.yandex_dns_zone.oncall.id
  type = each.value.ip_version == "ipv4" ? "A" : "AAAA"
  ttl = 200
  data = [
    each.value.ip_version == "ipv4"
    ? ycp_vpc_address.balancer[each.key].external_ipv4_address.0.address
    : ycp_vpc_address.balancer[each.key].ipv6_address.0.address
  ]
}

# Application Load Balancers

# See https://cloud.yandex.ru/docs/application-load-balancer/concepts/application-load-balancer#security-groups
resource "yandex_vpc_security_group" "dutybot_alb" {
  folder_id = var.folder_id

  network_id = var.network_id
  name = "dutybot-${var.environment}-alb-sg"
  description = "Security group for Dutybot load balancers [${var.environment}]"

  ingress {
    protocol = "ICMP"
    description = "Allow pings"
    v4_cidr_blocks = ["0.0.0.0/0"]
    port = "0"
  }

  ingress {
    protocol = "IPV6_ICMP"
    description = "Allow pings"
    v6_cidr_blocks = ["::/0"]
    port = "0"
  }

  ingress {
    protocol = "TCP"
    description = "Allow 30080/tcp from balancer nodes"
    v4_cidr_blocks = concat(
      ["198.18.235.0/24", "198.18.248.0/24"]
    )
    v6_cidr_blocks = concat(
      [var.hc_network_ipv6, "2a02:6b8::/32"]
    )
    port = 30080
  }

  ingress {
    protocol = "ANY"
    description = "Allow any ingress traffic from the internet to 80/tcp"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    port = 80
  }

  ingress {
    protocol = "ANY"
    description = "Allow any ingress traffic from the internet to 443/tcp"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    port = 443
  }

  egress {
    protocol = "ANY"
    description = "Allow any egress traffic"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    from_port = 0
    to_port = 65535
  }
}

# See https://wiki.yandex-team.ru/cloud/devel/platform-team/l7/usage/

resource "ycp_platform_alb_load_balancer" "dutybot" {
  for_each = local.messengers

  name = "dutybot-${each.key}-${var.environment}-alb"
  folder_id = var.folder_id

  listener {
    name = "http"

    endpoint {
      ports = [80]

      address {
        dynamic "external_ipv4_address" {
          for_each = each.value.ip_version == "ipv4" ? ["ipv4"] : []
          content {
            address = ycp_vpc_address.balancer[each.key].external_ipv4_address[0].address
          }
        }

        dynamic "external_ipv6_address" {
          for_each = each.value.ip_version == "ipv6" ? ["ipv6"] : []
          content {
            address = ycp_vpc_address.balancer[each.key].ipv6_address[0].address
            yandex_only = true
          }
        }
      }
    }

    http {
      redirects {
        http_to_https = true
      }
    }
  }

  listener {
    name = "tls"

    endpoint {
      ports = [443]

      address {
        dynamic "external_ipv4_address" {
          for_each = each.value.ip_version == "ipv4" ? ["ipv4"] : []
          content {
            address = ycp_vpc_address.balancer[each.key].external_ipv4_address[0].address
          }
        }

        dynamic "external_ipv6_address" {
          for_each = each.value.ip_version == "ipv6" ? ["ipv6"] : []
          content {
            address = ycp_vpc_address.balancer[each.key].ipv6_address[0].address
            yandex_only = true
          }
        }
      }
    }

    tls {
      default_handler {
        certificate_ids = [ycp_certificatemanager_certificate_request.dutybot[each.key].id]

        http_handler {
          allow_http10   = false
          http_router_id = ycp_platform_alb_http_router.dutybot[each.key].id
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
    dynamic location {
      for_each = var.subnet_ids
      content {
        subnet_id = location.value
        zone_id   = location.key

        disable_traffic = false
      }
    }
  }

  security_group_ids = [
    yandex_vpc_security_group.dutybot_alb.id
  ]

  # Enable Jaeger traces
  tracing {
    # Preprod: https://jaeger.private-api.ycp.cloud-preprod.yandex.net
    # Prod: https://jaeger.private-api.ycp.cloud.yandex.net
    service_name = "dutybot-${each.key}-${var.environment}"
  }

  # Cluster for metrics in the Solomon
  # Preprod: https://solomon.cloud-preprod.yandex-team.ru/?project=aoem2v5as6lv1ebgg1cu
  # Prod: https://solomon.cloud.yandex-team.ru/?project=b1grffd2lfm69s7koc4t
  solomon_cluster_name = "dutybot-${each.key}-${var.environment}"

  # Host for alerts in the Juggler
  juggler_host = each.value.domain

  depends_on = [
    ycp_microcosm_instance_group_instance_group.dutybot,
  ]
}

resource "ycp_platform_alb_http_router" "dutybot" {
  for_each = local.messengers
  name = "dutybot-${each.key}-${var.environment}-http-router"
  folder_id = var.folder_id
}

resource "ycp_platform_alb_virtual_host" "dutybot" {
  for_each = local.messengers

  name = "dutybot-${each.key}-${var.environment}-vhost"

  authority = [each.value.domain]
  http_router_id = ycp_platform_alb_http_router.dutybot[each.key].id

  route {
    name = "default"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = ycp_platform_alb_backend_group.dutybot[each.key].id
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "dutybot" {
  for_each = local.messengers

  name = "dutybot-${each.key}-${var.environment}-backend-group"
  folder_id = var.folder_id

  http {
    backend {
      name = "dutybot"

      port = each.value.port

      target_group {
        target_group_id = ycp_microcosm_instance_group_instance_group.dutybot.application_load_balancer_state[0].target_group_id
      }

      healthchecks {
        timeout             = "1s"
        interval            = "2s"
        healthy_threshold   = 4
        unhealthy_threshold = 1
        http {
          path = "/"
        }
      }
    }
  }
}
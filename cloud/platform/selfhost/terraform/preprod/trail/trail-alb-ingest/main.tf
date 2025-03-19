provider "ycp" {
  prod      = false
  folder_id = var.yc_folder
  zone      = var.yc_zone
  ycp_profile = "trail-preprod"
}

resource "ycp_certificatemanager_certificate_request" "trail_ingest_certificate_internal" {
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
  name = "trail-ingest-certificate-internal"
  domains = ["ingest.trail.cloud-preprod.yandex.net"]
  description = "Internal certificate for Cloud Trail Ingest API ALB (preprod)"
  folder_id   = var.yc_folder
  cert_provider = "INTERNAL_CA"
}

resource "ycp_platform_alb_target_group" "trail_ingest_target_group" {
  name        = "trail-ingest-target-group-preprod"
  description = "Target Group for Cloud Trail Ingest API ALB (preprod)"
  folder_id   = var.yc_folder

  labels      = {
    layer   = "paas"
    abc_svc = "yccloudtrail"
    env     = "pre-prod"
  }

  dynamic "target" {
    for_each = var.subnets

    content {
      ip_address = var.subnets_addresses[target.value]
      subnet_id  = target.value
      locality {
        zone_id = target.key
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "trail_ingest_backend_group" {
  name        = "trail-ingest-backend-group-preprod"
  description = "Backend Group for Cloud Trail Ingest API (preprod)"
  folder_id   = var.yc_folder
  labels      = {
    layer   = "paas"
    abc_svc = "yccloudtrail"
    env     = "pre-prod"
  }

  grpc {
    backend {
      name = "trail-ingest-api-backend-preprod"
      port = 443
      backend_weight = 100

      healthchecks {
        interval = "2s"
        timeout = "1s"
        healthy_threshold = 2
        unhealthy_threshold = 3
        healthcheck_port = 9982

        http {
          path = "/ping"
        }

        plaintext {}
      }

      passive_healthcheck {
        max_ejection_percent = 66
        consecutive_gateway_failure = 2
        base_ejection_time = "30s"
        enforcing_consecutive_gateway_failure = 100
        interval = "10s"
      }

      target_groups {
        target_group_ids = [ ycp_platform_alb_target_group.trail_ingest_target_group.id ]
      }

      tls {}
    }
  }
}

resource ycp_vpc_address trail_ingest_vpc_address {
  lifecycle {
    prevent_destroy = true
  }

  folder_id   = var.yc_folder
  name = "trail-ingest-l7-ipv6"

  ipv6_address_spec {
    requirements {
      hints = ["yandex-only"]
    }
  }
  reserved = true
}

# Каждый балансер - это 6 ВМ (3 на препроде), и хотя это не очень большие ВМ -
# желательно не создавать по балансеру на каждый домен,
# а иметь один балансер на все домены, и потом делать маршрутизацию по доменам.
# (Для публичного и приватного апи нужны отдельные балансеры)
resource ycp_platform_alb_load_balancer trail_ingest_alb_load_balancer {
  name = "trail-ingest-l7"

  folder_id   = var.yc_folder

  security_group_ids = var.security_group_ids

  close_traffic_policy = "CLOSE_TRAFFIC_POLICY_UNSPECIFIED"

  listener {
    name = "tls"

    endpoint {
      ports = [ 443 ]

      address {
        external_ipv6_address {
          # адрес созданный ранее
          address = ycp_vpc_address.trail_ingest_vpc_address.ipv6_address[0].address
          yandex_only = true
        }
      }
    }

    tls {
      default_handler {
        # ID сертификата в менеджере сертификатов
        certificate_ids = [ ycp_certificatemanager_certificate_request.trail_ingest_certificate_internal.id ]

        tls_options {
          tls_max_version = "TLS_AUTO"
          tls_min_version = "TLS_V1_2"
        }

        http_handler {
          # Роуты, см. ниже
          http_router_id = ycp_platform_alb_http_router.trail_ingest_alb_http_router.id

          # True если балансер для внешних пользователей.
          # (Этот флаг влияет только на генерацию новых заголовков x-request-id)
          is_edge = false

          http2_options {
            # Ограничение окон (буферов) HTTP/2 соединений, опционально.
            # По умолчанию окна по 256Мб, поэтому несколько HTTP/2 соединений могут занять всю ОЗУ,
            # если клиент или бэкэнд не будут успевать вычитывать данные.
            # При заполнении окна, писатель не сможет слать данные пока их не вычитает читатель.
            initial_stream_window_size     = 1024*1024
            initial_connection_window_size = 1024*1024
            # Также рекомендуется уменьшить количество стримов HTTP/2 соединения.
            max_concurrent_streams         = 100
          }
        }
      }
      tcp_options {
        # Ограничение размера буфера соединения, опционально.
        # Размер_буфера * количество_соединений < размер_ОЗУ.
        # Для публичных балансеров (куда ходят внешние недоверенные пользователи)
        # лучше выставить значение поменьше, например 32к.
        # Для приватных можно оставить значение по умолчанию, 1Мб.
        # Метрика в соломоне - alb_memory_allocated.
        connection_buffer_limit_bytes = 1024*1024*1024
      }
    }
  }

  region_id = var.yc_zone

  # ID сети, в которой будут размещены ВМ балансера.
  network_id = var.network_id

  # Список зон и сабнетов, в которых будут размещены ВМ балансера.
  allocation_policy {
    dynamic "location" {
      for_each = var.subnets
      content {
        zone_id   = location.key
        subnet_id = location.value
        disable_traffic = false
      }
    }
  }

  # Включить трейсы в егере.
  tracing {
    # Имя не должно пересекаться с другими сервисами.
    # Препрод: https://jaeger.private-api.ycp.cloud-preprod.yandex.net
    # Прод: https://jaeger.private-api.ycp.cloud.yandex.net
    service_name = "trail_ingest_alb-l7"
  }

  # Кластер для метрик в соломоне.
  # Препрод: https://solomon.cloud-preprod.yandex-team.ru/?project=aoem2v5as6lv1ebgg1cu
  # Прод: https://solomon.cloud-preprod.yandex-team.ru/?project=b1grffd2lfm69s7koc4t
  solomon_cluster_name = "trail_ingest_preprod_alb-l7"

  # Хост для алертов в juggler.
  juggler_host = "cloud_preprod_l7-trail_ingest_alb"
}

# Сюда будут привязываться virtual host'ы с роутами.
resource ycp_platform_alb_http_router trail_ingest_alb_http_router {
  name = "trail-ingest-alb-http-router"
  folder_id   = var.yc_folder
  # Других полей тут нет.
}


resource ycp_platform_alb_virtual_host trail_ingest_alb_virtual_host {
  name = "trail-ingest-alb-virtual-host-preprod"

  # Список масок хостов, которые матчить.
  # Хосты не должны пересекаться с другими virtual host в пределах балансера.
  authority = ["ingest.trail.cloud-preprod.yandex.net"]

  # Роутер (см. выше).
  http_router_id = ycp_platform_alb_http_router.trail_ingest_alb_http_router.id

  route {
    name = "main-route"

    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        auto_host_rewrite = false
        backend_group_id  = ycp_platform_alb_backend_group.trail_ingest_backend_group.id

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "DEADLINE_EXCEEDED",
          ]
        }
      }
    }
  }
}

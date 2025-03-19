resource "ycp_platform_alb_load_balancer" "alb" {
  name = var.name # имя самого балансера
  folder_id = var.folder_id

  listener {
    name = var.listener_name # имя лисенера, нужно чтобы найти лисенер при обновлении

    endpoint {
      # список портов на которых принимать запросы
      ports = var.endpoint_ports

      dynamic address {
        for_each = var.external_ipv6_addresses
        content {
          external_ipv6_address {
            # адрес созданный ранее
            address = address.value
          }
        }
      }

      dynamic address {
        for_each = var.external_ipv4_addresses
        content {
          external_ipv4_address {
            # адрес созданный ранее
            address = address.value
          }
        }
      }
    }

    tls {
      default_handler {
        # ID сертификатов в менеджере сертификатов
        certificate_ids = var.certificate_ids

        http_handler {
          # Роуты, см. ниже
          http_router_id = var.https_router_id

          # True если балансер для внешних пользователей.
          # (Этот флаг влияет только на генерацию новых заголовков x-request-id)
          is_edge = var.is_edge

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

      dynamic sni_handlers {
        for_each = var.sni_handlers
        content {
          name         = sni_handlers.value.name
          server_names = sni_handlers.value.server_names

          handler {
            certificate_ids = sni_handlers.value.certificate_ids

            http_handler {
              http_router_id = sni_handlers.value.http_router_id
              is_edge        = sni_handlers.value.is_edge
            }
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
        connection_buffer_limit_bytes = 32768
      }
    }
  }

  dynamic listener {
    for_each = (var.http_router_id == null ? [] : [var.http_router_id])

    content {
      name = "http"

      endpoint {
        ports = [80]

        dynamic address {
          for_each = var.external_ipv6_addresses
          content {
            external_ipv6_address {
              address = address.value
            }
          }
        }

        dynamic address {
          for_each = var.external_ipv4_addresses
          content {
            external_ipv4_address {
              address = address.value
            }
          }
        }
      }

      http {
        handler {
          http_router_id = var.http_router_id
          is_edge        = var.is_edge
          allow_http10   = true
        }
      }
    }
  }

  # ID сети, в которой будут размещены ВМ балансера.
  network_id = var.allocation.network_id

  region_id = var.allocation.region_id

  # Список зон и сабнетов, в которых будут размещены ВМ балансера.
  allocation_policy {
    dynamic location {
      for_each = var.allocation.locations
      content {
        disable_traffic = false
        subnet_id       = location.value.subnet_id
        zone_id         = location.value.zone_id
      }
    }
  }

  # Включить трейсы в егере.
  tracing {
    # Имя не должно пересекаться с другими сервисами.
    # Препрод: https://jaeger.private-api.ycp.cloud-preprod.yandex.net
    # Прод: https://jaeger.private-api.ycp.cloud.yandex.net
    service_name = var.name
  }

  # Кластер для метрик в соломоне.
  # Препрод: https://solomon.cloud-preprod.yandex-team.ru/?project=aoem2v5as6lv1ebgg1cu
  # Прод: https://solomon.cloud-preprod.yandex-team.ru/?project=b1grffd2lfm69s7koc4t
  solomon_cluster_name = var.solomon_cluster_name

  # Хост для алертов в juggler.
  juggler_host = var.juggler_host
}

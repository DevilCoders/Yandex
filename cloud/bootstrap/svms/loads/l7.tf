resource ycp_vpc_address load-selfhost-alb {
  lifecycle {
    prevent_destroy = true # не даем удалять адрес чтобы не пришлось менять ДНС
  }

  name      = "load-CLOUD-71275"
  folder_id = local.folder_id

  ipv6_address_spec {
    requirements {
      hints = ["yandex-only"]
    }
  }
  reserved = true # убирает лишний diff в terraform plan
}

resource "ycp_dns_dns_record_set" load-selfhost-alb {
  zone_id = local.dns_zone_id
  name    = "load-selfhost-alb"
  type    = "AAAA"
  ttl     = "3600"
  data    = [ycp_vpc_address.load-selfhost-alb.ipv6_address[0].address]
}

resource ycp_platform_alb_load_balancer load-selfhost-alb {
  name      = local.alb_name # имя самого балансера
  folder_id = local.folder_id

  listener {
    name = "http" # имя лисенера, нужно чтобы найти лисенер при обновлении

    endpoint {
      ports = [80] # список портов на которых принимать запросы

      address {
        external_ipv6_address {
          # адрес созданный ранее
          address = ycp_vpc_address.load-selfhost-alb.ipv6_address[0].address
        }
      }
    }

    http {
      handler {
        allow_http10   = false
        http_router_id = ycp_platform_alb_http_router.load-selfhost-alb.id

        # True если балансер для внешних пользователей.
        # (Этот флаг влияет только на генерацию новых заголовков x-request-id)
        is_edge = false

        http2_options {
          # Ограничение окон (буферов) HTTP/2 соединений, опционально.
          # По умолчанию окна по 256Мб, поэтому несколько HTTP/2 соединений могут занять всю ОЗУ,
          # если клиент или бэкэнд не будут успевать вычитывать данные.
          # При заполнении окна, писатель не сможет слать данные пока их не вычитает читатель.
          initial_stream_window_size     = 1024 * 1024
          initial_connection_window_size = 1024 * 1024
          # Также рекомендуется уменьшить количество стримов HTTP/2 соединения.
          max_concurrent_streams = 100
        }
      }
      tcp_options {
        # Ограничение размера буфера соединения, опционально.
        # Размер_буфера * количество_соединений < размер_ОЗУ.
        # Для публичных балансеров (куда ходят внешние недоверенные пользователи)
        # лучше выставить значение поменьше, например 32к.
        # Для приватных можно оставить значение по умолчанию, 1Мб.
        # Метрика в соломоне - alb_memory_allocated.
        # connection_buffer_limit_bytes = 32768
      }
    }
  }

  region_id = "ru-central1"

  # ID сети, в которой будут размещены ВМ балансера.
  network_id = local.network_id

  # Список зон и сабнетов, в которых будут размещены ВМ балансера.
  allocation_policy {
    location {
      subnet_id = local.subnet_ids["ru-central1-a"].id
      zone_id   = "ru-central1-a"

      # Этот флаг выключает трафик в зоне.
      disable_traffic = false
    }
  }

  # Кластер для метрик в соломоне.
  # Препрод: https://solomon.cloud-preprod.yandex-team.ru/?project=aoem2v5as6lv1ebgg1cu
  # Прод: https://solomon.cloud-preprod.yandex-team.ru/?project=b1grffd2lfm69s7koc4t
  solomon_cluster_name = local.alb_name

  # Хост для алертов в juggler.
  juggler_host = "cloud_preprod_l7-load-cloud-71275"
}

# Сюда будут привязываться virtual host'ы с роутами.
resource ycp_platform_alb_http_router load-selfhost-alb {
  name      = "load-cloud-71275-l7"
  folder_id = local.folder_id
}

resource ycp_platform_alb_virtual_host load-selfhost-alb {
  name = "default"

  # Список масок хостов, которые матчить.
  # Хосты не должны пересекаться с другими virtual host в пределах балансера.
  authority = ["*"]

  # Роутер (см. выше).
  http_router_id = ycp_platform_alb_http_router.load-selfhost-alb.id

  route {
    name = local.alb_name

    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        # Бэкэкнд, см. ниже.
        backend_group_id = ycp_platform_alb_backend_group.load-selfhost-alb.id
      }
    }
  }
}

resource ycp_platform_alb_backend_group load-selfhost-alb {
  name      = local.alb_name
  folder_id = local.folder_id

  http { # или grpc
    backend {
      name = local.alb_name

      # Порт бэкэндов.
      port = 80

      # Если надо включить TLS.
      # При этом хелсчеки тоже будут использовать TLS.
      # tls {}
      # выставить в -1 если нужно отключить backend
      weight = 1

      target_group {
        target_group_id = "a5d0po7p9qvh1r2086pd"
      }

      healthchecks {
        # Таймаут должен быть таким чтобы бэкэнд успел за него ответить под нагрузкой.
        timeout             = "0.5s"
        interval            = "2s"
        healthy_threshold   = 2
        unhealthy_threshold = 2
        http {
          path = "/ping"
        }
      }
    }
    backend {
      name   = "${local.alb_name}-underlay"
      weight = -1

      # Порт бэкэндов.
      port = 80

      # Если надо включить TLS.
      # При этом хелсчеки тоже будут использовать TLS.
      # tls {}

      target {
        endpoint {
          external_address = true
          ip_address       = "2a02:6b8:bf00:41:9cbe:8ff:fe56:572"
          subnet_id        = ""
          port             = 80
        }
      }

      healthchecks {
        # Таймаут должен быть таким чтобы бэкэнд успел за него ответить под нагрузкой.
        timeout             = "0.5s"
        interval            = "2s"
        healthy_threshold   = 2
        unhealthy_threshold = 2
        http {
          path = "/ping"
        }
      }
    }
  }
}

/*
# Создание L7 балансера


Рекомендуемый способ - использование ((https://wiki.yandex-team.ru/cloud/devel/terraform-ycp/ приватного терраформа)) последней версии. 

*/

terraform {
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">= 0.34"
    }
  }
  required_version = ">= 0.13"
}

provider "ycp" {
  prod        = false
  ycp_profile = "preprod"

  cloud_id  = "aoeolbmrnipjt7p7kf6p" # ycloud-platform
  folder_id = "aoernscbbg560jvhk69a" # l7-demo
}

/*

## 0) Пререквизиты: 

- наличие облака с IPv6 сетью. Ноды балансера будут находиться в этой сети, и через неё ходить в бекэнды. Если IPv6 сети нет, ((https://wiki.yandex-team.ru/cloud/for-yteam/ её можно заказать тут)) (вместе с отдельным облаком).

- дырки от HCaaS (_CLOUD_LB_HC_PREPROD_NETS_) до этой сети на порт 30080 - для хелсчеков нод балансера. Как минимум нужна дырка в панчере. Если в облаке включены группы безопасности, то в них тоже надо дырку. ((https://wiki.yandex-team.ru/cloud/security/research/cloud-security-groups/ См. документацию.))

- фича-флаг ALB_ALPHA на облаке (проставляется в backoffice).

- для TLS (https) надо будет создать сертификат и положить его в серт. менеджер. ((https://wiki.yandex-team.ru/cloud/devel/certificate-manager/ См. документацию.))

*/

locals {
  network_id = "c64vqqt3eb7ls0chrj0t"   # platform-nets
  subnet_ids = ["bucpba0hulgrkgpd58qp"] # platform-nets-ru-central1-a
}

/*

## 1) Создание адреса балансера

Хотя л7 балансер и может сам выделить адрес (через л3 балансер), лучше выделить адрес заранее.
На этот адрес потом можно будет навесить фичи типа анти-ддос.

Днс-запись надо будет навесить самостоятельно, возможно через новый днс-сервис. ((https://wiki.yandex-team.ru/cloud/devel/ycdns/ См. документацию.))

*/

resource "ycp_vpc_address" "this" {
  lifecycle {
    prevent_destroy = true # не даем удалять адрес чтобы не пришлось менять ДНС
  }

  name = "demo-l7-ipv6"

  ipv6_address_spec {
    requirements {
      hints = [] # или ["yandex-only"] если надо yandex-only адрес.
    }
  }
  reserved = true # убирает лишний diff в terraform plan
}

/*

## 1a) Создание серта и днс записи

Т.к. заведение днс-зоны через терраформ пока невозможно, а тестовой днс зоны у нас нет, создавать днс запись мы не будем.

*/

resource "ycp_certificatemanager_certificate_request" "this" {
  name = "demo-l7"
  domains = [
    "demo-l7.ycp.cloud-preprod.yandex.net"
  ]
  cert_provider  = "INTERNAL_CA"
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}

/*

## 2) Создание балансера.

*/

resource "ycp_platform_alb_load_balancer" "this" {
  name = "demo-l7" # имя самого балансера

  listener {
    name = "tls" # имя лисенера, нужно чтобы найти лисенер при обновлении

    endpoint {
      ports = [443] # список портов на которых принимать запросы

      address {
        external_ipv6_address {
          # адрес созданный ранее
          address = ycp_vpc_address.this.ipv6_address[0].address
        }
      }
    }

    tls {
      default_handler {
        # ID сертификата в менеджере сертификатов
        certificate_ids = [ycp_certificatemanager_certificate_request.this.id]

        http_handler {
          # Роуты, см. ниже
          http_router_id = ycp_platform_alb_http_router.this.id

          # True если балансер для внешних пользователей.
          # (Этот флаг влияет только на генерацию новых заголовков x-request-id)
          is_edge = true

          http2_options {
            # Определяется экспериментально.
            # Если окна не хватает, то запросы будут виснуть.
            initial_stream_window_size     = 1048576
            initial_connection_window_size = 1024 * 1024
            max_concurrent_streams         = 100
          }
        }
      }
      tcp_options {
        # Определяется экспериментально.
        # Размер_буфера * количество_соединений < размер_ОЗУ.
        # Если ОЗУ не хватает, обращайтесь к дежурному l7.
        # Метрика в соломоне - alb_memory_allocated
        connection_buffer_limit_bytes = 32768
      }
    }
  }

  region_id = "ru-central1"

  # ID сети, в которой будут размещены ВМ балансера.
  network_id = local.network_id

  # Список зон и сабнетов, в которых будут размещены ВМ балансера.
  allocation_policy {
    location {
      subnet_id = local.subnet_ids[0]
      zone_id   = "ru-central1-a"

      # Этот флаг выключает трафик в зоне.
      disable_traffic = false
    }
  }

  # Включить трейсы в егере.
  tracing {
    # Имя не должно пересекаться с другими сервисами.
    # Препрод: https://jaeger.private-api.ycp.cloud-preprod.yandex.net
    # Прод: https://jaeger.private-api.ycp.cloud.yandex.net
    service_name = "demo-l7"
  }

  # Кластер для метрик в соломоне.
  # Препрод: https://solomon.cloud-preprod.yandex-team.ru/?project=aoem2v5as6lv1ebgg1cu
  solomon_cluster_name = "demo-l7"

  # Хост для алертов в juggler.
  juggler_host = "cloud_preprod_l7-demo"
}

# Сюда будут привязываться virtual host'ы с роутами.
resource "ycp_platform_alb_http_router" "this" {
  name = "demo-l7"
  # Других полей тут нет.
}

/*

## 3) Прописываем роуты.

Роуты описывают как матчить входящие запросы и в какие бэкэнды их направлять.
Также тут можно настроить таймауты (60сек по умолчанию), ретраи, поддежку веб-сокетов и т.п.
((https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/platform/alb/v1/http_router.proto См. прото-спеку.))

*/

resource "ycp_platform_alb_virtual_host" "this" {
  name = "default"

  # Список масок хостов, которые матчить.
  # Хосты не должны пересекаться с другими virtual host в пределах балансера.
  authority = ["*"]

  # Роутер (см. выше).
  http_router_id = ycp_platform_alb_http_router.this.id

  route {
    name = "default"

    http { # или grpc
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        # Бэкэкнд, см. ниже.
        backend_group_id = ycp_platform_alb_backend_group.this.id
      }
    }
  }
}

/*

## 4) Прописываем бэкенды.

Бэкенды описывают как балансировать запросы, хелсчеки, etc.
((https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/platform/alb/v1/backend_group.proto См. прото-спеку.))

*/

resource "ycp_platform_alb_backend_group" "this" {
  name = "demo-l7"

  http { # или grpc
    backend {
      name = "default"

      port = 8000

      target_group {
        # Инстанс-группа сама может создать таргет группу.
        target_group_id = ycp_microcosm_instance_group_instance_group.this.application_load_balancer_state[0].target_group_id
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

/*

## 5) Таргет-группа.

Рекомендуется использовать инстанс-группу. Она сама создаст таргет-группу.

*/

resource "ycp_microcosm_instance_group_instance_group" "this" {
  name = "demo-backend"

  service_account_id = "bfb73m3egaqqbdngto5m" # l7-demo

  instance_template {
    metadata = {
      user-data = <<-EOT
      #cloud-config
      users:
      - name: demoserver
        sudo: False
      write_files:
      - path: /home/demoserver/ping
        content: OK
      - path: /usr/bin/local/demoserver.py
        content: |
          from http.server import HTTPServer
          from http.server import SimpleHTTPRequestHandler
          import socket

          class Srv(HTTPServer):
              address_family = socket.AF_INET6

          server = Srv(('::', 8000), SimpleHTTPRequestHandler)
          server.serve_forever()
      - path: /etc/systemd/system/demoserver.service
        content: |
          [Unit]
          Wants=network-online.target
          After=network-online.target
          StartLimitIntervalSec=0
          [Service]
          Type=simple
          Restart=always
          RestartSec=
          User=demoserver
          WorkingDirectory=/home/demoserver
          ExecStart=/usr/bin/python3 /usr/bin/local/demoserver.py
          [Install]
          WantedBy=multi-user.target
      runcmd:
      - 'chown -R demoserver:demoserver /home/demoserver'
      - systemctl enable demoserver
      - systemctl start demoserver
      EOT
    }

    platform_id = "standard-v2"

    boot_disk {
      mode = "READ_WRITE"
      disk_spec {
        image_id = "fdvoujtrnj85g623bvrv" # ubuntu-1804-lts-1614851036
        size     = 10
        type_id  = "network-hdd"
      }
    }

    network_interface {
      network_id = local.network_id
      subnet_ids = local.subnet_ids

      primary_v4_address { # metadata needs IPv4
        name = "ig-v4addr"
      }
      primary_v6_address {
        name = "ig-v6addr"
      }
    }

    resources {
      core_fraction = 20
      cores         = 2
      memory        = 1
    }

  }

  application_load_balancer_spec {
    target_group_spec {
      address_names = ["ig-v6addr"]
    }
  }
  platform_l7_load_balancer_spec {
    preferred_ip_version = "IP_VERSION_UNSPECIFIED"
    target_group_spec {
      address_names = ["ig-v6addr"]
    }
  }

  allocation_policy {
    zone {
      zone_id = "ru-central1-a"
    }
  }

  deploy_policy {
    max_unavailable = 1
  }

  scale_policy {
    fixed_scale {
      size = 1
    }
  }
}

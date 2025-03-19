# ycp_platform_alb_load_balancer.this:
resource "ycp_platform_alb_load_balancer" "this" {
  # id = "ds740vlv9e7rmajpehrd"
  lifecycle {
    prevent_destroy = true
    ignore_changes  = [modified_at]
  }
  
  folder_id = "b1gvgqhc57450av0d77p"

  description = "Private API"
  name        = "cpl"

  # internal       = true
  # traffic_scheme = "TRAFFIC_SCHEME_UNSPECIFIED"

  deletion_protection = false

  labels = {
    "internal-conductor-group" = "l7-cpl"
    "internal-instance-name"   = "cpl-router-ig"
    "internal-solomon-cluster" = "cloud_prod_cpl"

    "internal-enable-remote-als" = "yyy"
    "internal-disable-local-als" = "yyy"
  }

  solomon_cluster_name = "cloud_prod_cpl"
  juggler_host         = "cloud_prod_l7-cpl-router"
  security_group_ids   = []

  listener {
    name = "frontend-tls"

    endpoint {
      ports = [443]

      address {
        external_ipv6_address {
          yandex_only = true
        }
      }
    }

    tls {
      default_handler {
        certificate_ids = ["fpqd0esoafiebucbiffm"]

        http_handler {
          http_router_id = "ds7v7mmcfkhqsgn034s6"
        }

        tls_options {
          cipher_suites   = []
          ecdh_curves     = []
          tls_max_version = "TLS_AUTO"
          tls_min_version = "TLS_V1_2"
        }
      }

      sni_handlers {
        name = "logbroker-preprod"
        server_names = [
          "logbroker-preprod.cloud.yandex.ru",
        ]

        handler {
          certificate_ids = ["fpqlg0u6ve8ld6cdhtnh"]

          http_handler {
            http_router_id = "ds72khnrmjksgsi9mvuc"
            is_edge        = true
          }

          tls_options {
            cipher_suites   = []
            ecdh_curves     = []
            tls_max_version = "TLS_AUTO"
            tls_min_version = "TLS_V1_2"
          }
        }
      }
      sni_handlers {
        name = "logbroker-prod"
        server_names = [
          "logbroker.cloud.yandex.ru",
        ]

        handler {
          certificate_ids = ["fpqp56i4d0b18v4udgnd"]

          http_handler {
            http_router_id = "ds7e05k5b9qd7jnhg96e"
            is_edge        = true
          }

          tls_options {
            cipher_suites   = []
            ecdh_curves     = []
            tls_max_version = "TLS_AUTO"
            tls_min_version = "TLS_V1_2"
          }
        }
      }
      sni_handlers {
        name = "auth-cloud-yt-ru"
        server_names = [
          "auth.cloud.yandex-team.ru",
        ]

        handler {
          certificate_ids = ["fpqlct0pvgn6urbh3o15"]

          http_handler {
            http_router_id = "ds7vprfj1tegf0g5opvn"
            is_edge        = true
          }

          tls_options {
            cipher_suites   = []
            ecdh_curves     = []
            tls_max_version = "TLS_AUTO"
            tls_min_version = "TLS_V1_2"
          }
        }
      }
      sni_handlers {
        name = "monitoring-charts-staging"
        server_names = [
          "monitoring-charts-staging.private-api.cloud.yandex.net",
        ]

        handler {
          certificate_ids = ["fpqc4jr0tgahuqdi0liq"]

          http_handler {
            http_router_id = "ds7ijgm6dd34j0tj0etn"
            is_edge        = true
          }

          tls_options {
            cipher_suites   = []
            ecdh_curves     = []
            tls_max_version = "TLS_AUTO"
            tls_min_version = "TLS_V1_2"
          }
        }
      }
      sni_handlers {
        name = "monitoring-charts"
        server_names = [
          "monitoring-charts.private-api.cloud.yandex.net",
        ]

        handler {
          certificate_ids = ["fpq1528v69enfd81udof"]

          http_handler {
            http_router_id = "ds7ve73qjvc3k13qari6"
            is_edge        = false
          }
        }
      }
      sni_handlers {
        name = "mk8s-marketplace"
        server_names = [
          "mk8s-marketplace.private-api.ycp.cloud.yandex.net",
        ]
        handler {
          certificate_ids = ["fpqd0esoafiebucbiffm"]

          http_handler {
            http_router_id = "ds7isqcj3ju8js6uq3b1"
          }
        }
      }

      tcp_options {
        connection_buffer_limit_bytes = 1048576 # 1M
      }
    }
  }

  allocation_policy {
    location {
      disable_traffic = false
      subnet_id       = "e9bi9drqb7rddf4od5jk"
      zone_id         = "ru-central1-a"
    }
    location {
      disable_traffic = false
      subnet_id       = "e2lo6auqjjfru2tfnorm"
      zone_id         = "ru-central1-b"
    }
    location {
      disable_traffic = false
      subnet_id       = "b0c3id3uje06qohkgmlg"
      zone_id         = "ru-central1-c"
    }
  }
}

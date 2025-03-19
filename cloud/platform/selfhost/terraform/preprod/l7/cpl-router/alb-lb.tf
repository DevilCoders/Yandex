resource "ycp_platform_alb_load_balancer" "this" {
  # id = "a5d1ki8mi4a8pugs0b74"
  folder_id = "aoe4lof1sp0df92r6l8j"

  lifecycle {
    prevent_destroy = true
    ignore_changes  = [modified_at]
  }

  # internal       = true
  # traffic_scheme = "TRAFFIC_SCHEME_UNSPECIFIED"

  name        = "cpl"
  description = "Private API"

  deletion_protection = false

  labels = {
    "internal-conductor-group" = "l7-cpl"
    "internal-instance-name"   = "cpl-preprod"

    "internal-enable-remote-als" = "yyy"
    "internal-disable-local-als" = "yyy"

    "internal-enable-tcp-user-timeout" = "yyy" # CLOUD-58106
  }

  juggler_host         = "cloud_preprod_l7-cpl-router"
  solomon_cluster_name = "cloud_preprod_cpl"

  listener {
    name = "frontend-tls"

    endpoint {
      ports = [443]

      address {
        external_ipv6_address {
          # 2a0d:d6c0:0:ff1b::27d // it might be not safe to fill the address field now
          yandex_only = true
        }
      }
    }

    tls {
      default_handler {
        certificate_ids = [ycp_certificatemanager_certificate_request.this.id]

        http_handler {
          http_router_id = ycp_platform_alb_http_router.cpl-router.id
          is_edge        = false
        }
        tls_options {
          tls_min_version = "TLS_V1_2"
          tls_max_version = "TLS_AUTO"
        }
      }

      sni_handlers {
        name = "monitoring"
        server_names = [
          "monitoring-preprod.cloud.yandex.ru",
          "monitoring-preprod.cloud.yandex.com",
          "cloud-monitoring-preprod.cloud.yandex.ru",
        ]

        handler {
          certificate_ids = ["fd3a8lqfljcq53cf17g1"]

          http_handler {
            http_router_id = "a5dfnpo37sgg61b58534"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "monitoring-charts"
        server_names = [
          "monitoring-charts.private-api.cloud-preprod.yandex.net",
        ]

        handler {
          certificate_ids = ["fd3g6lt4h2lmi4gg36ce"]

          http_handler {
            http_router_id = "a5dc19budnn3k6h4k7ad"
          }
        }
      }
      # https://st.yandex-team.ru/CLOUD-71942
      sni_handlers {
        name = "mk8s-marketplace"
        server_names = [
          "mk8s-marketplace.private-api.ycp.cloud-preprod.yandex.net",
        ]

        handler {
          certificate_ids = [ycp_certificatemanager_certificate_request.this.id]

          http_handler {
            http_router_id = "a5dso7iptibt9cdl5fpe"
            is_edge        = false
          }
        }
      }
    }
  }

  allocation_policy {
    location {
      disable_traffic = false
      subnet_id       = "bucqe7hncapq5rrr6os3"
      zone_id         = "ru-central1-a"
    }
    location {
      disable_traffic = false
      subnet_id       = "blto554kooj9q9qbs2l4"
      zone_id         = "ru-central1-b"
    }
    location {
      disable_traffic = false
      subnet_id       = "fo2doum42mfhj1nea6sp"
      zone_id         = "ru-central1-c"
    }
  }
}

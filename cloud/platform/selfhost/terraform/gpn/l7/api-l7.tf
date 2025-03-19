# ycp_platform_alb_load_balancer.api:
resource "ycp_platform_alb_load_balancer" "api" {
  # id = "albu97e2a18hqq439aia"

  name        = "api-router"
  description = "Public API"
  labels = {
    "internal-conductor-group" = "l7-api-router"
    "internal-instance-name"   = "api-router"
  }

  internal = true

  security_group_ids   = []
  solomon_cluster_name = "cloud_gpn_api-router"
  traffic_scheme       = "TRAFFIC_SCHEME_UNSPECIFIED"

  listener {
    name = "frontend-tls"

    endpoint {
      ports = [443]

      address {
        external_ipv4_address {}
      }
      address {
        external_ipv6_address {}
      }
    }

    tls {
      default_handler {
        certificate_ids = ["cert-api-yandex"]

        http_handler {
          http_router_id = ycp_platform_alb_http_router.api.id
          is_edge        = true
        }
      }

      sni_handlers {
        name = "local"
        server_names = [
          // old domains
          "api.yac.techpark.local",
          "*.api.yac.techpark.local",
          "console.yac.techpark.local",
          "serialws.yac.techpark.local",
          "cr.yac.techpark.local",
          // new domains
          "api.yac.devzone.local",
          "*.api.yac.devzone.local",
          "console.yac.devzone.local",
          "serialws.yac.devzone.local",
          "cr.yac.devzone.local",
        ]

        handler {
          certificate_ids = ["cert-api-gpn"]

          http_handler {
            http_router_id = ycp_platform_alb_http_router.api.id
            is_edge        = true
          }
        }
      }

      sni_handlers {
        name = "auth-ya"
        server_names = [
          "auth.gpn.yandexcloud.net",
        ]

        handler {
          certificate_ids = ["cert-auth-yandex"]

          http_handler {
            http_router_id = "fke8at599onft1nb40nk"
            is_edge        = true
          }
        }
      }

      sni_handlers {
        name = "auth-local"
        server_names = [
          "auth.yac.devzone.local",
        ]

        handler {
          certificate_ids = ["cert-auth-gpn"]

          http_handler {
            http_router_id = "fke8at599onft1nb40nk"
            is_edge        = true
          }
        }
      }
    }
  }

  allocation_policy {
    location {
      subnet_id = "e57mq3uf1nimamcnoahu"
      zone_id   = "ru-gpn-spb99"
    }
  }
}

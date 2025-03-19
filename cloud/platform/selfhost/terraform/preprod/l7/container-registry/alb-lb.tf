# ycp_platform_alb_load_balancer.this:
resource "ycp_platform_alb_load_balancer" "this" {
  # id                   = "a5db4qtedt404kf5g2ih"

  internal = true

  folder_id = "aoe7os3b67d3dn79fv5p"

  name        = "container-registry"
  description = "Container registry L7"

  labels = {
    "internal-conductor-group" = "l7-container-registry"
    "internal-instance-name"   = "container-registry-l7-preprod"
    
    "internal-enable-remote-als" = "true"
    "internal-disable-local-als" = "yyy"
  }

  persistent_logs = false

  juggler_host         = "cloud_preprod_l7-container-registry"
  solomon_cluster_name = "cloud_preprod_container_registry"

  traffic_scheme = "TRAFFIC_SCHEME_UNSPECIFIED"

  listener {
    name = "tls"

    endpoint {
      ports = [
        443,
      ]

      address {
        external_ipv4_address {}
      }
      address {

        external_ipv6_address {
          yandex_only = false
        }
      }
    }

    tls {
      default_handler {
        http_handler {
          allow_http10   = false
          http_router_id = "a5dnset9o78kjp1ur781"
          is_edge        = true

          http2_options {
            initial_connection_window_size = 1048576
            initial_stream_window_size     = 1048576
            max_concurrent_streams         = 50
          }
        }

        certificate_ids = [
          "fd3fn8hd09u06qg9dek9",
        ]

        tls_options {
          tls_max_version = "TLS_AUTO"
          tls_min_version = "TLS_V1_2"
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

  security_group_ids = [
    "c64gqadee5ce1l8jlgbo",
  ]
}

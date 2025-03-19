resource "ycp_platform_alb_load_balancer" "this" {
  # id = "a5d2fd2m46a3rh7kja53"
  folder_id = "aoe4lof1sp0df92r6l8j"

  internal       = true
  traffic_scheme = "TRAFFIC_SCHEME_UNSPECIFIED"

  name        = "jaeger"
  description = "LB for jaeger"

  deletion_protection = false

  labels = {
    "internal-conductor-group" = "l7-jaeger"
    "internal-instance-name"   = "jaeger-preprod"
    "internal-no-tracing"      = "yes"

    "internal-enable-remote-als" = "yyy"
    "internal-disable-local-als" = "yyy"
  }

  juggler_host         = "cloud_preprod_l7-jaeger"
  solomon_cluster_name = "cloud_preprod_jaeger"

  listener {
    name = "tls"

    endpoint {
      ports = [443]

      address {
        external_ipv6_address {}
      }
    }

    tls {
      default_handler {
        certificate_ids = [ycp_certificatemanager_certificate_request.this.id]

        http_handler {
          http_router_id = "a5dho02q66j5969lp8di"
          is_edge        = true
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

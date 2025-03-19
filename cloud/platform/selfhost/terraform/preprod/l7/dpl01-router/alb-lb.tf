resource "ycp_platform_alb_load_balancer" "this" {
  # id = "a5don06t7fst0hrvvhar"
  folder_id = "aoe4lof1sp0df92r6l8j"

  internal       = true
  traffic_scheme = "TRAFFIC_SCHEME_UNSPECIFIED"

  name        = "dpl01"
  description = "Translate API"

  deletion_protection = false

  labels = {
    "internal-conductor-group" = "l7-dpl01-router"
    "internal-instance-name"   = "dpl01-preprod"
    "internal-solomon-cluster" = "cloud_preprod_dpl01"

    "internal-enable-remote-als" = "yyy"
  }

  close_traffic_policy = "CLOSE_TRAFFIC_POLICY_UNSPECIFIED"

  juggler_host         = "cloud_preprod_l7-dpl01"
  solomon_cluster_name = "cloud_preprod_dpl01"

  listener {
    name = "tls"

    endpoint {
      ports = [443]

      address {
        external_ipv4_address {}
      }
      address {
        external_ipv6_address {
          # 2a0d:d6c0:0:ff1b::311
        }
      }
    }

    tls {
      default_handler {
        certificate_ids = [ycp_certificatemanager_certificate_request.this.id]

        http_handler {
          http_router_id = ycp_platform_alb_http_router.dpl01-router.id
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

resource "ycp_platform_alb_load_balancer" "this" {
  # id = "a5d1sj0fgesrkvp9rmno"
  folder_id = "aoe4lof1sp0df92r6l8j"

  name        = "staging"
  description = "LB for tests"
  labels = {
    "internal-conductor-group" = "l7-staging"
    "internal-instance-name"   = "staging-preprod"
    "internal-solomon-cluster" = "cloud_preprod_staging"

    "internal-enable-remote-als" = "true"
    "internal-disable-local-als" = "yyy"

    "internal-enable-tcp-user-timeout" = "yyy" # CLOUD-58106
  }

  deletion_protection = false
  internal            = true

  security_group_ids = []

  traffic_scheme = "TRAFFIC_SCHEME_UNSPECIFIED"

  juggler_host         = "cloud_preprod_l7-staging"
  solomon_cluster_name = "cloud_preprod_staging"

  persistent_logs = false

  allocation_policy {
    location {
      disable_traffic = false
      subnet_id       = "bucqe7hncapq5rrr6os3"
      zone_id         = "ru-central1-a"
    }
  }

  listener {
    name = "tls"

    endpoint {
      ports = [443]
      address {
        external_ipv6_address {
          # 2a0d:d6c0:0:ff1a::a8
          yandex_only = false
        }
      }
    }

    tls {
      default_handler {
        certificate_ids = [ycp_certificatemanager_certificate_request.this.id]

        http_handler {
          allow_http10   = false
          http_router_id = ycp_platform_alb_http_router.staging.id
          is_edge        = true
        }
      }
    }
  }
  listener {
    name = "plaintext"

    endpoint {
      ports = [80]

      address {
        external_ipv6_address {
          yandex_only = false
        }
      }
    }

    http {
      redirects {
        http_to_https = true
      }
    }
  }
}

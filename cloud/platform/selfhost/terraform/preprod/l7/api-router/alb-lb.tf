# ycp_platform_alb_load_balancer.this:
resource "ycp_platform_alb_load_balancer" "this" {
  # id = "a5df2gob0ht92segfftv"
  folder_id = "aoe4lof1sp0df92r6l8j"

  internal       = true
  traffic_scheme = "TRAFFIC_SCHEME_UNSPECIFIED"

  name        = "api-router"
  description = "Public API"

  deletion_protection = false

  labels = {
    "internal-conductor-group" = "l7-api-router"
    "internal-instance-name"   = "api-router-preprod"
    "internal-solomon-cluster" = "cloud_preprod_api-router"

    "internal-enable-remote-als"       = "yyy"
    "internal-disable-local-als"       = "yyy"
    "internal-enable-tcp-user-timeout" = "yyy" # CLOUD-58106
  }

  juggler_host         = "cloud_preprod_l7-api-router"
  solomon_cluster_name = "cloud_preprod_api-router"

  persistent_logs = false

  security_group_ids = [
    "c6470ufg2h245kum3hhc",
  ]

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
        certificate_ids = [ycp_certificatemanager_certificate_request.this.id]

        http_handler {
          http_router_id = ycp_platform_alb_http_router.api-router.id
          is_edge        = true
        }
      }

      sni_handlers {
        name = "datalens"
        server_names = [
          "preprod.datalens.yandex",
          "datalens-preprod.yandex.ru",
        ]

        handler {
          certificate_ids = ["fd3s5mio8eu3hdlsv8pp"]

          http_handler {
            http_router_id = "a5dsvalakhqb6l8ea2r6"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "datalens-upload"
        server_names = [
          "upload.datalens-preprod.yandex.ru",
          "upload.datalens-test.yandex.ru",
        ]

        handler {
          certificate_ids = ["fd3keneq9i7qv7nhhpg6"]

          http_handler {
            http_router_id = "a5diji6g0bbnk2skp350"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "org-yandex"
        server_names = [
          "org-preprod.yandex.ru",
          "org-preprod.yandex.com",
          "org-preprod.cloud.yandex.ru",
          "org-preprod.cloud.yandex.com",
        ]

        handler {
          certificate_ids = ["fd3ef9dbvhlmqa047kav"]

          http_handler {
            http_router_id = "a5dj0hckjco005licc28"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "auth-preprod"
        server_names = [
          "auth-preprod.cloud.yandex.ru",
          "auth-preprod.cloud.yandex.com",
        ]

        handler {
          certificate_ids = ["fd3bv1b4ttjmnf2opemv"]

          http_handler {
            http_router_id = "a5dbov27nq1glt45nman"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "portal-preprod"
        server_names = [
          "portal-preprod.cloud.yandex.ru",
          "partners-preprod.cloud.yandex.ru",
          "cloud-portal-preprod.cloud.yandex.ru",
        ]

        handler {
          certificate_ids = ["fd3lbcicgl6mfu27dfi2"]

          http_handler {
            http_router_id = "a5dr2rv9jklg6o80nbpt"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "support-preprod"
        server_names = [
          "support-preprod.cloud.yandex.ru",
        ]

        handler {
          certificate_ids = ["fd37hrvs6hlgpr3q2r88"]

          http_handler {
            http_router_id = "a5daldtj94lb91lt3462"
            is_edge        = true
          }
        }
      }
      # https://st.yandex-team.ru/CLOUD-69876
      sni_handlers {
        name = "billing-preprod"
        server_names = [
          "billing-preprod.cloud.yandex.ru"
        ]

        handler {
          certificate_ids = ["fd3hnj5afrkmd7k2qos1"]

          http_handler {
            http_router_id = "a5dclghfasiscemuoqqp"
          }
        }
      }

      # https://st.yandex-team.ru/CLOUD-66532
      sni_handlers {
        name = "server-yfm-preprod"
        server_names = [
          "server-yfm-preprod.cloud.yandex.net",
        ]

        handler {
          certificate_ids = ["fd3nbcgn7p9ollk06mnt"]

          http_handler {
            http_router_id = "a5d1elcah4mrb6mfejgq"
            is_edge        = true
          }
        }
      }
    }
  }
  listener {
    name = "plaintext"

    endpoint {
      ports = [80]

      address {
        external_ipv4_address {}
      }
      address {
        external_ipv6_address {}
      }
    }

    http {
      handler {
        allow_http10   = true # TBD: do we still use l3tt ?
        http_router_id = ycp_platform_alb_http_router.plaintext.id
        is_edge        = true
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

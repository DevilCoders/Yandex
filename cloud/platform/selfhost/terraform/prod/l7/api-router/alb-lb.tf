# ycp_platform_alb_load_balancer.this:
resource "ycp_platform_alb_load_balancer" "this" {
  # id = "ds7mj163e7hk59k3p2je"
  folder_id = "b1gvgqhc57450av0d77p"

  name        = "api-router"
  description = "Public API"

  internal       = true
  traffic_scheme = "TRAFFIC_SCHEME_UNSPECIFIED"

  labels = {
    "internal-conductor-group" = "l7-api-router"
    "internal-instance-name"   = "api-router-ig"
    "internal-solomon-cluster" = "cloud_prod_api-router"

    "internal-enable-remote-als" = "yyy"
    "internal-disable-local-als" = "yyy"
  }

  juggler_host         = "cloud_prod_l7-api-router"
  solomon_cluster_name = "cloud_prod_api-router"
  deletion_protection  = true

  security_group_ids = []

  close_traffic_policy = "CLOSE_TRAFFIC_POLICY_UNSPECIFIED"

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
      tcp_options {
        connection_buffer_limit_bytes = 131072 # 128K
      }

      default_handler {
        certificate_ids = [ycp_certificatemanager_certificate_request.this.id]

        http_handler {
          allow_http10   = false
          http_router_id = "ds76jvo202hvcbitb1f0"
          is_edge        = true
        }
      }

      sni_handlers {
        name = "org"
        server_names = [
          "org.yandex.ru",
          "org.yandex.com",
          "org.cloud.yandex.ru",
          "org.cloud.yandex.com",
        ]

        handler {
          certificate_ids = ["fpqnvde3ccjrbre7ogkk"]

          http_handler {
            allow_http10   = false
            http_router_id = "ds7m0sot4qoll25vfckj"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "datalens"
        server_names = [
          "datalens.yandex.ru",
          "datalens-staging.yandex.ru",
          "datalens.yandex",
          "staging.datalens.yandex",
          "upload.datalens.yandex.ru",
          "upload.datalens.yandex.net",
          "datalens.yandex.com",
          "datalens-staging.yandex.com",
          "upload.datalens.yandex.com",
        ]

        handler {
          certificate_ids = ["fpqt1no8ckosotdf31ic"]

          http_handler {
            allow_http10   = false
            http_router_id = "ds73d7bfuams7a7f5abi"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "dataproc-ui"
        server_names = [
          "dataproc-ui.yandexcloud.net",
          "*.dataproc-ui.yandexcloud.net",
        ]

        handler {
          certificate_ids = ["fpqii047tq3gtfj0gtfj"]

          http_handler {
            allow_http10   = false
            http_router_id = "ds7k0vatrph5h5fsjp0n"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "auth"
        server_names = [
          "auth.cloud.yandex.ru",
          "auth.cloud.yandex.com",
        ]

        handler {
          certificate_ids = ["fpqba99cg8nm7do73g4r"]

          http_handler {
            allow_http10   = false
            http_router_id = "ds7nlis9u7k72vmh4oac"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "partner-portal"
        server_names = [
          "partners.cloud.yandex.ru",
          "cloud-partners.cloud.yandex.ru",
        ]

        handler {
          certificate_ids = ["fpqf1d1kn0h6uar34p7j"]

          http_handler {
            allow_http10   = false
            http_router_id = "ds7c13n1dmjhu6hon76b"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "monitoring-staging"
        server_names = [
          "monitoring-staging.cloud.yandex.ru",
          "monitoring-staging.cloud.yandex.com",
        ]

        handler {
          certificate_ids = ["fpqr4rnrjam0v55jbqg8"]

          http_handler {
            allow_http10   = false
            http_router_id = "ds71dmgo45kgp03emm87"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "monitoring-ui"
        server_names = [
          "monitoring.cloud.yandex.ru",
          "monitoring.cloud.yandex.com",
        ]

        handler {
          certificate_ids = ["fpqvgntmk88g71qg3dqi"]

          http_handler {
            allow_http10   = false
            http_router_id = "ds7l89o9o5sk343kd45o"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "support-prod"
        server_names = [
          "support.cloud.yandex.ru",
          "support.cloud.yandex.com",
        ]

        handler {
          certificate_ids = ["fpq2lksdjgpu9fvt5im1"]

          http_handler {
            allow_http10   = false
            http_router_id = "ds72meqaqe6cuuqch4aq"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "org-assessors"
        server_names = [
          "org-assessors.cloud.yandex.ru",
          "org-assessors.cloud.yandex.com",
        ]

        handler {
          certificate_ids = ["fpq83s77npqedpjudoca"]

          http_handler {
            http_router_id = "ds7e7g90fdgpk8rrlkmd"
            is_edge        = true
          }
        }
      }
      sni_handlers {
        name = "billing"
        server_names = [
          "billing.cloud.yandex.ru",
          "billing.cloud.yandex.com",
        ]

        handler {
          certificate_ids = ["fpqdegvqur4aae0bp7hu"]

          http_handler {
            http_router_id = "ds75j5iv9ngcb3vhfsfj"
            is_edge        = true
          }
        }
      }
      # https://st.yandex-team.ru/CLOUD-73547
      sni_handlers {
        name = "server-yfm"
        server_names = [
          "server-yfm.cloud.yandex.net",
        ]

        handler {
          certificate_ids = ["fpqqrssfeam4r9avt8ii"]

          http_handler {
            http_router_id = "ds775eevg07tgc3m7gak"
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
        external_ipv6_address {}
      }
      address {
        external_ipv4_address {}
      }
    }

    http {
      handler {
        allow_http10   = true
        http_router_id = "ds7bk7l1vse12umrielf"
        is_edge        = true
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

resource "ycp_platform_alb_http_router" "staging" {
  # id = "a5d5ug919fjgp75ehssu"

  name        = "staging"
  description = "router for tests"
  folder_id   = "aoe4lof1sp0df92r6l8j"
}

resource "ycp_platform_alb_http_router" "staging-plaintext" {
  # id = "a5d7hht47lqbalb58k57"

  name        = "staging-plaintext"
  description = "router for tests"
  folder_id   = "aoe4lof1sp0df92r6l8j"
}

resource "ycp_platform_alb_virtual_host" "staging-plaintext-vh" {
  authority = ["*"]

  name = "staging-plaintext-vh"

  http_router_id = ycp_platform_alb_http_router.staging-plaintext.id

  route {
    name = "test"
    http {
      match {
        path {
          prefix_match = "/test"
        }
      }
      direct_response {
        status = 200
        body {
          text = "test"
        }
      }
    }
  }

  route {
    name = "https-redirect"
    http {
      redirect {
        replace_scheme = "https"
        response_code  = "MOVED_PERMANENTLY"
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "echo-websocket-org" {
  #id = "a5dv161an33ai0d3ojol"

  name        = "echo-websocket-org"
  description = "echo-websocket-org"
  folder_id   = "aoe4lof1sp0df92r6l8j"
  labels      = {}

  http {
    backend {
      name = "echo-websocket-org-backend"

      allow_connect = false
      port          = 80
      use_http2     = false
      weight        = 100

      passive_healthcheck {
        base_ejection_time                    = "30s"
        consecutive_gateway_failure           = 2
        enforcing_consecutive_gateway_failure = 100
        interval                              = "10s"
        max_ejection_percent                  = 66
      }

      target {
        endpoint {
          hostname = "echo.websocket.org"
        }
      }
    }

    connection {}
  }
}

resource "ycp_platform_alb_backend_group" "local" {
  name      = "local"
  folder_id = "aoe4lof1sp0df92r6l8j"

  http {
    backend {
      name = "local"

      weight = 100

      target {
        endpoint {
          hostname = "127.0.0.1"
          port     = 8080
        }
      }
    }

    connection {}
  }
}

resource "ycp_platform_alb_virtual_host" "vh-websocket-test" {
  # id             = "a5d5ug919fjgp75ehssu/vh-websocket-test"

  authority = [
    "websock.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.staging.id
  name           = "vh-websocket-test"
  ports = [
    443,
  ]

  route {
    name = "proxy_route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = ycp_platform_alb_backend_group.echo-websocket-org.id
        host_rewrite       = "echo.websocket.org"
        support_websockets = true
      }
    }
  }
}

# Test VH that rotutes to the api-gateway.
resource "ycp_platform_alb_virtual_host" "staging-api-gateway" {
  authority = [
    "api.ycp.cloud-preprod.yandex.net",
  ]

  name = "api-gateway"

  http_router_id = ycp_platform_alb_http_router.staging.id

  route {
    name = "main"

    grpc {
      route {
        backend_group_id = "a5d94ag15u9bhgl9fkkb" # api-canary-backend-group-preprod
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "functions" {
  # id = "a5d9gb9ast23jg07eh3u"

  name      = "functions"
  folder_id = "aoe4lof1sp0df92r6l8j"
  labels    = {}

  http {
    backend {
      name = "functions"

      # tls {}
      backend_weight = 100

      target {
        endpoint {
          hostname = "functions.cloud-preprod.yandex.net"
          port     = 80
        }
      }
    }

    connection {}
  }
}

# Example:
#
#  curl -s https://f.l7-staging.ycp.cloud-preprod.yandex.net/b093sj7bnul8k9t8s748?test=1
#
resource "ycp_platform_alb_virtual_host" "functions" {
  authority = [
    "f.l7-staging.ycp.cloud-preprod.yandex.net",
  ]

  name = "functions"

  http_router_id = ycp_platform_alb_http_router.staging.id

  route {
    name = "main"

    http {
      route {
        host_rewrite     = "functions.cloud-preprod.yandex.net"
        backend_group_id = ycp_platform_alb_backend_group.functions.id
      }
    }
  }
}

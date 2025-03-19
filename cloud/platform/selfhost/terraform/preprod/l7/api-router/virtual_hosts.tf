# ycp_platform_alb_virtual_host.vh-ui:
resource "ycp_platform_alb_virtual_host" "vh-ui" {
  authority = [
    "console-preprod.cloud.yandex.ru",
    "console-preprod.cloud.yandex.com",
    "console.front-preprod.cloud.yandex.ru",
    "console.front-preprod.cloud.yandex.com",
  ]
  http_router_id = "a5d48mbh1i5o8dudgs0v"
  name           = "vh-ui"
  ports = [
    443,
  ]

  modify_request_headers {
    name    = "X-Forwarded-Host"
    remove  = false
    replace = "%REQ(:authority)%"
  }

  route {
    name = "invokeFunction"

    http {

      match {
        path {
          prefix_match = "/_/serverless/invokeFunction/"
        }
      }

      route {
        timeout      = "0s"
        idle_timeout = "305s"

        backend_group_id   = "a5dpfq9tt6udlsqjlv1d"
        support_websockets = true
      }
    }
  }

  route {
    name = "main_route"

    http {

      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        backend_group_id   = "a5dpfq9tt6udlsqjlv1d"
        support_websockets = true
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-dataproc-ui" {
  authority = [
    "*.dataproc-ui.cloud-preprod.yandex.net",
    "dataproc-ui.cloud-preprod.yandex.net",
  ]
  http_router_id = "a5d48mbh1i5o8dudgs0v"
  name           = "vh-dataproc-ui"
  ports = [
    443,
  ]

  modify_request_headers {
    name    = "X-Forwarded-Host"
    remove  = false
    replace = "%REQ(:authority)%"
  }

  route {
    name = "main_route"

    http {
      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        backend_group_id = "a5do0igqp9qlqcsuqc0j"
      }
    }
  }
}

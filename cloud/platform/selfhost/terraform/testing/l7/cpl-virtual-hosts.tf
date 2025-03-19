resource "ycp_platform_alb_http_router" "cpl" {
  # id = albdifjqmtjirn0merkb
  name = "cpl"
}

resource "ycp_platform_alb_virtual_host" "vh-instance-group-testing" {
  authority = [
    "instance-group.private-api.ycp.cloud-testing.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl.id

  name  = "vh-instance-group-testing"
  ports = [443]

  route {
    name = "main_route"

    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }

      route {
        backend_group_id = "albfacu9ri5h03pg5bmn"
        max_timeout      = "60s"

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "DEADLINE_EXCEEDED",
          ]
        }
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-api-adapter-testing" {
  authority = [
    "api-adapter.private-api.ycp.cloud-testing.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl.id

  name  = "vh-api-adapter-testing"
  ports = [443]

  route {
    name = "main_route"

    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }

      route {
        backend_group_id = "albcq8stqe126c9b425f"
        max_timeout      = "60s"

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "DEADLINE_EXCEEDED",
          ]
        }
      }
    }
  }
}

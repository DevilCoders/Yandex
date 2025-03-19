resource "ycp_platform_alb_http_router" "plaintext" {
  # id = "ds7bk7l1vse12umrielf"
  name = "plaintext"
}

resource "ycp_platform_alb_virtual_host" "plaintext" {
  authority = ["*"]

  name = "plaintext"

  http_router_id = ycp_platform_alb_http_router.plaintext.id

  route {
    name = "ping"
    http {
      match {
        path {
          exact_match = "/ping"
        }
      }

      direct_response {
        status = 200
        body {
          text = "OK"
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

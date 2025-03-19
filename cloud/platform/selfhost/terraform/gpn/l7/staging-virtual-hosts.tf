resource "ycp_platform_alb_http_router" "staging" {
  # id = fke8jcoh6f86tm0slii4
  name = "staging"
}

resource "ycp_platform_alb_virtual_host" "staging-test" {
  authority      = ["staging.private-api.ycp.gpn.yandexcloud.net"]
  http_router_id = ycp_platform_alb_http_router.staging.id
  name           = "staging-test"

  route {
    name = "admin"

    http {
      match {
        path {
          exact_match = "/memory"
        }
      }

      route {
        backend_group_id = ycp_platform_alb_backend_group.admin.id
      }
    }
  }

  route {
    name = "default"

    http {
      direct_response {
        status = 200
        body {
          text = "OK"
        }
      }
    }
  }
}

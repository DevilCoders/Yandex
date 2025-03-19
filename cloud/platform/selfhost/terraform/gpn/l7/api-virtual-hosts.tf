resource "ycp_platform_alb_http_router" "api" {
  # id = albka1k7lnf79a65bc28
  name = "api-router"
}

resource "ycp_platform_alb_virtual_host" "api-test" {
  authority      = ["test.api.gpn.yandexcloud.net"]
  http_router_id = ycp_platform_alb_http_router.api.id
  name           = "api-test"

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

resource "ycp_platform_alb_virtual_host" "vh-ui-console-gpn" {
  name      = "vh-ui-console-gpn"
  authority = ["console.gpn.yandexcloud.net", "console.yac.techpark.local", "console.yac.devzone.local"]
  ports     = [443]

  http_router_id = ycp_platform_alb_http_router.api.id
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
        auto_host_rewrite  = false
        backend_group_id   = "fkeejcpijpdh4jclvvu4"
        idle_timeout       = "60s"
        support_websockets = true
        timeout            = "60s"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-serialws-gpn" {
  name      = "vh-serialws-gpn"
  authority = ["serialws.gpn.yandexcloud.net", "serialws.yac.techpark.local", "serialws.yac.devzone.local"]
  ports     = [443]

  http_router_id = ycp_platform_alb_http_router.api.id
  route {
    name = "main_route"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/ws/console/"
        }
      }
      route {
        prefix_rewrite     = "/"
        auto_host_rewrite  = false
        backend_group_id   = "fkerbmi03c15vereg3n7"
        idle_timeout       = "600s"
        support_websockets = true
        timeout            = "600s"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-cr-dpl-gpn" {
  name      = "vh-cr-dpl-gpn"
  authority = ["cr.gpn.yandexcloud.net", "cr.yac.techpark.local", "cr.yac.devzone.local"]
  ports     = [443]

  http_router_id = ycp_platform_alb_http_router.api.id
  route {
    name = "viewer_health"

    http {
      direct_response {
        status = 404

        body {
          text = jsonencode(
            {
              errors = [
                {
                  code    = "PATH_NOT_FOUND"
                  message = "Route /health/ is unknown"
                },
              ]
            }
          )
        }
      }

      match {
        http_method = [
          "GET",
          "HEAD",
        ]

        path {
          prefix_match = "/health/"
        }
      }
    }
  }
  route {
    name = "viewer_get_head"

    http {

      match {
        http_method = [
          "GET",
          "HEAD",
        ]

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "fkebcfs2a52e5n4os576"
        idle_timeout       = "60s"
        support_websockets = false
        timeout            = "60s"

        retry_policy {
          num_retries = 2
          retry_on = [
            "RETRY_5XX",
          ]
        }
      }
    }
  }
  route {
    name = "uploader_put_post_patch_delete"

    http {

      match {
        http_method = [
          "PUT",
          "POST",
          "PATCH",
          "DELETE",
        ]

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "fke7djkk8bbp2c6fk96q"
        idle_timeout       = "60s"
        support_websockets = false
        timeout            = "60s"

        retry_policy {
          num_retries = 2
          retry_on = [
            "CONNECT_FAILURE",
            "REFUSED_STREAM",
          ]
        }
      }
    }
  }
  route {
    name = "unknown_methods"

    http {
      direct_response {
        status = 405

        body {
          text = jsonencode(
            {
              errors = [
                {
                  code    = "UNSUPPORTED"
                  message = "Method is not allowed, please check that method and uri are correct"
                },
              ]
            }
          )
        }
      }

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }
    }
  }
}

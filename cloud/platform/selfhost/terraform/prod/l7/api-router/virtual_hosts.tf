# ycp_platform_alb_virtual_host.vh-for-l7-ui-console:
resource "ycp_platform_alb_virtual_host" "vh-for-l7-ui-console" {
  authority = [
    "console.cloud.yandex.ru",
    "console.cloud.yandex.com",
    "console.front-extprod.cloud.yandex.ru",
    "console.front-extprod.cloud.yandex.com",
  ]
  http_router_id = "ds76jvo202hvcbitb1f0"
  name           = "vh-for-l7-ui-console"
  ports = [
    443,
  ]

  modify_request_headers {
    name    = "X-Forwarded-Host"
    remove  = false
    replace = "%REQ(:authority)%"
  }

  route {
    name = "aiSendAudio"

    http {

      match {
        path {
          prefix_match = "/_/ai/sendAudio"
        }
      }

      route {
        timeout      = "0s"
        idle_timeout = "330s" # 5m30s

        backend_group_id   = "ds79nt7mkk0ahakk9s2g"
        support_websockets = true
      }
    }
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
        idle_timeout = "305s" # 5m5s

        backend_group_id   = "ds79nt7mkk0ahakk9s2g"
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
        backend_group_id   = "ds79nt7mkk0ahakk9s2g"
        support_websockets = true
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-datalens-staging-gateway:
resource "ycp_platform_alb_virtual_host" "vh-datalens-staging-gateway" {
  authority = [
    "datalens-assessors.yandex.ru",
  ]
  http_router_id = "ds76jvo202hvcbitb1f0"
  name           = "vh-datalens-staging-gateway"
  ports = [
    443,
  ]

  route {
    name = "wizard-preview"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/wizard/preview/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7ehmfgsupg9kqkm7n3"
        prefix_rewrite     = "/"
        support_websockets = false
      }
    }
  }
  route {
    name = "wizard"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/wizard/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7jfqrlctk288kck9hn"
        prefix_rewrite     = "/"
        support_websockets = false
      }
    }
  }
  route {
    name = "gateway"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/gateway/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7hhju0l75odbalpa83"
        prefix_rewrite     = "/"
        support_websockets = false
      }
    }
  }
  route {
    name = "dashboards"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/dashboards/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7l075ne0aqj8qd1eb6"
        prefix_rewrite     = "/"
        support_websockets = false
      }
    }
  }
  route {
    name = "datasets"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/datasets"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7sia9jihr7jcv45v5o"
        support_websockets = false
      }
    }
  }
  route {
    name = "connections"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/connections"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7sia9jihr7jcv45v5o"
        support_websockets = false
      }
    }
  }
  route {
    name = "charts"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/charts/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7sr7kil7fk5ud8b794"
        prefix_rewrite     = "/"
        support_websockets = false
      }
    }
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
        auto_host_rewrite  = false
        backend_group_id   = "ds794egqcrs0ccec5phl"
        support_websockets = false
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-datalens-staging-upload:
resource "ycp_platform_alb_virtual_host" "vh-datalens-staging-upload" {
  authority = [
    "upload.datalens-assessors.yandex.ru",
  ]
  http_router_id = "ds76jvo202hvcbitb1f0"
  name           = "vh-datalens-staging-upload"
  ports = [
    443,
  ]

  route {
    name = "main_route"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/api"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds766k4a8uj6j537osdk"
        support_websockets = false
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-for-l7-ui-console-staging:
resource "ycp_platform_alb_virtual_host" "vh-for-l7-ui-console-staging" {
  authority = [
    "console-staging.cloud.yandex.ru",
  ]
  http_router_id = "ds76jvo202hvcbitb1f0"
  name           = "vh-for-l7-ui-console-staging"
  ports = [
    443,
  ]

  route {
    name = "aiSendAudio"

    http {

      match {
        path {
          prefix_match = "/_/ai/sendAudio"
        }
      }

      route {
        timeout      = "0s"
        idle_timeout = "330s" # 5m30s

        backend_group_id   = "ds7fkuud72i5rionppoo"
        support_websockets = true
      }
    }
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
        idle_timeout = "305s" # 5m5s

        backend_group_id   = "ds7fkuud72i5rionppoo"
        support_websockets = true
      }
    }
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
        auto_host_rewrite  = false
        backend_group_id   = "ds7fkuud72i5rionppoo"
        support_websockets = false
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-ui-l7-prod:
resource "ycp_platform_alb_virtual_host" "vh-ui-l7-prod" {
  authority = [
    "console-assessors.cloud.yandex.ru",
    "console-assessors.cloud.yandex.com",
  ]
  http_router_id = "ds76jvo202hvcbitb1f0"
  name           = "vh-ui-l7-prod"
  ports = [
    443,
  ]

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
        backend_group_id   = "ds78b34e8bknecgd15r6"
        idle_timeout       = "60s"
        support_websockets = false
        timeout            = "60s"
      }
    }
  }
}

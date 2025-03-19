terraform {
  required_providers {
    # Update on Linux:
    # $ curl https://mcdev.s3.mds.yandex.net/terraform-provider-ycp/install.sh | bash
    # Update on Mac:
    # $ brew upgrade terraform-provider-ycp
    ycp = ">= 0.7"
  }
}

provider "ycp" {
  prod      = true
  cloud_id  = "b1g3clmedscm7uivcn4a"
  folder_id = "b1gvgqhc57450av0d77p"
}

# ycp_platform_alb_http_router.staging ds7a2pu8erpjvdnb6kvs
resource "ycp_platform_alb_http_router" "staging" {
  name                = "staging"
  description         = "router for tests"
  folder_id           = "b1gvgqhc57450av0d77p"
  https_redirect      = false
  https_redirect_port = 0
}

resource "ycp_platform_alb_backend_group" "local" {
  name      = "local"
  folder_id = "b1gvgqhc57450av0d77p"

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

resource "ycp_platform_alb_virtual_host" "staging-local" {
  name = "local"
  authority = [
    "local.l7-staging.ycp.cloud.yandex.net",
  ]

  http_router_id = ycp_platform_alb_http_router.staging.id

  route {
    name = "local"

    http {
      match {
        path { prefix_match = "/" }
      }

      route {
        backend_group_id = ycp_platform_alb_backend_group.local.id
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "virtual_host" {
  name           = var.name
  authority      = var.authority
  http_router_id = var.https_router_id
  ports          = var.https_ports

  dynamic "modify_request_headers" {
    for_each = var.modify_request_headers == null ? [] : [1]
    content {
      name    = var.modify_request_headers.name
      remove  = var.modify_request_headers.remove
      replace = var.modify_request_headers.replace
    }
  }

  dynamic "route" {
    for_each = var.routes
    content {
      name = route.value.name

      dynamic grpc {
        for_each = route.value.is_http ? [] : [1]
        content {
          match {
            fqmn {
              prefix_match = route.value.prefix_match
            }
          }
          route {
            backend_group_id = route.value.backend_group_id
          }
        }
      }

      dynamic http {
        for_each = route.value.is_http ? [1] : []
        content {
          match {
            path {
              prefix_match = route.value.prefix_match
            }
          }
          route {
            backend_group_id = route.value.backend_group_id
          }
        }
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "redirect_to_ssl" {
  count = var.http_router_id == null ? 0 : 1

  name           = "redirect-to-ssl"
  authority      = var.authority
  http_router_id = var.http_router_id
  ports          = [80]

  route {
    name = "redirect-to-ssl"
    http {
      redirect {
        replace_port   = 443
        replace_scheme = "https"
        response_code  = "MOVED_PERMANENTLY"
      }
    }
  }
}

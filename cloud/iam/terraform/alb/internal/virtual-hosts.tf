module "iam_virtual_hosts" {
  source = "../modules/virtual_host/v1"

  for_each = {for k, v in local.iam_service_settings : k => v if k != "oauth"}

  name           = format("iam-ya-%s-vh", lookup(each.value, "virtual_host_name_suffix", each.key))
  authority      = each.value.authority
  https_router_id = ycp_platform_alb_http_router.default.id

  routes = [
    {
      name             = "main"
      is_http          = each.value.is_http
      prefix_match     = "/"
      backend_group_id = module.iam_service_backend_groups[each.key].backend_group_id
    }
  ]
}

resource "ycp_platform_alb_virtual_host" "vh-auth-yandex-team" {
  authority      = local.iam_service_settings["oauth"].authority
  http_router_id = ycp_platform_alb_http_router.auth["https"].id
  name           = "vh-auth-yandex-team"

  modify_request_headers {
    name    = "X-Forwarded-Host"
    remove  = false
    replace = "%REQ(:authority)%"
  }

  route {
    name = "oauth"

    http {
      match {
        http_method = []

        path {
          prefix_match = "/oauth/"
        }
      }

      route {
        backend_group_id = module.iam_service_backend_groups["oauth"].backend_group_id
      }
    }
  }
}

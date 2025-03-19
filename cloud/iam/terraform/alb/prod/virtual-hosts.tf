module "iam_auth_virtual_host_prod" {
  source = "../modules/virtual_host/v1"

  name            = "vh-auth"
  authority       = local.iam_service_settings_prod["oauth"].authority
  https_router_id = ycp_platform_alb_http_router.auth["https"].id
  https_ports     = [
    443,
  ]

  modify_request_headers = {
    name    = "X-Forwarded-Host"
    remove  = false
    replace = "%REQ(:authority)%"
  }

  routes = [
    {
      name             = "federations-logout"
      is_http          = true
      prefix_match     = "/federations/logout"
      backend_group_id = local.auth_ui_backend_group.id
    },
    {
      name             = "federations-endpoint"
      is_http          = true
      prefix_match     = "/federations/"
      backend_group_id = module.iam_service_backend_groups_prod["oauth"].backend_group_id
    },
    {
      name             = "oauth-route"
      is_http          = true
      prefix_match     = "/oauth/"
      backend_group_id = module.iam_service_backend_groups_prod["oauth"].backend_group_id
    },
    {
      name             = "auth-ui"
      is_http          = true
      prefix_match     = "/"
      backend_group_id = local.auth_ui_backend_group.id
    },
  ]
}

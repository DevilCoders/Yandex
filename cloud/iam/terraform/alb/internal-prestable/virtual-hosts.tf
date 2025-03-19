module "iam_virtual_hosts" {
  source = "../modules/virtual_host/v1"

  for_each = local.iam_service_settings

  name           = format("iam-ya-prestable-%s-vh", lookup(each.value, "virtual_host_name_suffix", each.key))
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

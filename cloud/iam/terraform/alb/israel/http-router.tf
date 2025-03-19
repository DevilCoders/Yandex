resource "ycp_platform_alb_http_router" "auth" {
  lifecycle {
    prevent_destroy = true
  }
  for_each = toset([
    "https",
    "http",
  ])

  name        = format("auth-%s-router-israel", each.key)
  description = format("Root %s router for cloud auth server (israel)", each.key)
  folder_id   = local.openid_folder.id
}

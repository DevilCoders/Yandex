resource "ycp_platform_alb_http_router" "auth" {
  lifecycle {
    prevent_destroy = true
  }
  for_each = toset([
    "https",
    "http",
  ])

  name        = format("auth-%s-router-hw-blue-lab", each.key)
  description = format("Root %s router for cloud auth server (hw-blue-lab)", each.key)
  folder_id   = local.openid_folder.id
}

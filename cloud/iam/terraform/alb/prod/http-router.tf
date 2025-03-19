resource "ycp_platform_alb_http_router" "auth" {
  # id = "ds7nlis9u7k72vmh4oac"
  lifecycle {
    prevent_destroy = true
  }
  for_each = toset([
    "https",
  ])

  name        = format("auth-%s-router-prod", each.key)
  description = format("Root %s router for cloud auth server (prod)", each.key)
  folder_id   = "yc.iam.service-folder"
}

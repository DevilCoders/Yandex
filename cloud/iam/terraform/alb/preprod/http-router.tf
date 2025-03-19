# Сюда будут привязываться virtual host'ы с роутами.
resource "ycp_platform_alb_http_router" "default" {
  # id = a5dhkhbj1ubvg5716uqs
  lifecycle {
    prevent_destroy = true
  }

  name = "auth-l7-default"
  # Других полей тут нет.
}

resource "ycp_platform_alb_http_router" "auth-router-preprod" {
  # id = "a5dbov27nq1glt45nman"
  lifecycle {
    prevent_destroy = true
  }
  for_each = toset([
    "https",
  ])

  name        = format("auth-%s-router-preprod", each.key)
  description = format("Root %s router for cloud auth server (preprod)", each.key)
  folder_id   = "yc.iam.service-folder"
}

resource "ycp_platform_alb_http_router" "auth-router-testing" {
  # id = "a5dpq7gu9netvbpr0j9j"
  lifecycle {
    prevent_destroy = true
  }
  for_each = toset([
    "https",
  ])

  name        = format("auth-%s-router-testing", each.key)
  description = format("Root %s router for cloud auth server (testing)", each.key)
  folder_id   = local.openid_folder.id
}

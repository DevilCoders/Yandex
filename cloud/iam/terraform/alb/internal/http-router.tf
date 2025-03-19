# Сюда будут привязываться virtual host'ы с роутами.
resource ycp_platform_alb_http_router default {
    # id = ds7qjun8hvqbidqihusn
    name      = "iam-ya-l7-default"
    folder_id = local.iam_ya_prod_folder.id
}

resource "ycp_platform_alb_http_router" "auth" {
  # id = "ds7vprfj1tegf0g5opvn"
  lifecycle {
    prevent_destroy = true
  }
  for_each = toset([
    "https",
  ])

  name        = format("auth-%s-router-yandex-team", each.key)
  description = format("Root %s router for cloud auth server (yandex-team)", each.key)
  folder_id   = local.iam_ya_prod_folder.id
}

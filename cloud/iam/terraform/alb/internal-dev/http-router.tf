# Сюда будут привязываться virtual host'ы с роутами.
resource ycp_platform_alb_http_router default {
  # id = ds7lksvrfnljr9v3q817
  name        = "iam-ya-dev-l7-default"
  description = "Default router for (yandex-team internal-dev)"
  folder_id   = local.iam_ya_dev_folder.id
}

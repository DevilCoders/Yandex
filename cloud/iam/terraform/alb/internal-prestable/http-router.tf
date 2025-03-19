# Сюда будут привязываться virtual host'ы с роутами.
resource ycp_platform_alb_http_router default {
  # id = ds7cd4qhr6aose43872v
  name        = "iam-ya-prestable-l7-default"
  description = "Default router for (yandex-team internal-prestable)"
  folder_id   = local.iam_ya_prestable_folder.id
}

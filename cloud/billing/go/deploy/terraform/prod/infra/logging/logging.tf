resource "yandex_logging_group" "piper-logs" {
  folder_id        = var.yc_folder
  name             = "piper-logs"
  description      = "piper serivice logs"
  retention_period = "72h0m0s"
}

output "piper-logs" {
  value = {
    id               = yandex_logging_group.piper-logs.id
    name             = yandex_logging_group.piper-logs.name
    retention_period = yandex_logging_group.piper-logs.retention_period
  }
}

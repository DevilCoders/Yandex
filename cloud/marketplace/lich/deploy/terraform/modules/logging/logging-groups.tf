locals {
  target_folder = var.folder_id
}

resource "yandex_logging_group" "marketplace-default" {
  name      = "marketplace-default"
  description = "Unseparated output from backends"

  folder_id = local.target_folder
  retention_period = "72h"
}

resource "yandex_logging_group" "marketplace-access-logs" {
  name      = "marketplace-access-log"
  description = "Nginx access logs"

  folder_id = local.target_folder
  retention_period = "72h"
}

resource "yandex_logging_group" "marketplace-backends" {
  name      = "marketplace-backends"
  description = "Backends logs: private API and workers"

  folder_id = local.target_folder
  retention_period = "72h"
}

resource "yandex_logging_group" "marketplace-fabrica" {
  name      = "marketplace-fabrica"
  description = "Fabrica logs"

  folder_id = local.target_folder
  retention_period = "72h"
}


output "access-logs" {
  value = {
    id               = yandex_logging_group.marketplace-access-logs.id
    name             = yandex_logging_group.marketplace-access-logs.name
    retention_period = yandex_logging_group.marketplace-access-logs.retention_period
  }
}

output "backends" {
  value = {
    id               = yandex_logging_group.marketplace-backends.id
    name             = yandex_logging_group.marketplace-backends.name
    retention_period = yandex_logging_group.marketplace-backends.retention_period
  }
}

output "fabrica" {
  value = {
    id               = yandex_logging_group.marketplace-fabrica.id
    name             = yandex_logging_group.marketplace-fabrica.name
    retention_period = yandex_logging_group.marketplace-fabrica.retention_period
  }
}

output "default-logs" {
  value = {
    id               = yandex_logging_group.marketplace-default.id
    name             = yandex_logging_group.marketplace-default.name
    retention_period = yandex_logging_group.marketplace-default.retention_period
  }
}

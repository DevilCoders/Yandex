locals {
  billing_service_folder = var.yc_folder
  network_name           = "billing-nets"
  zones = toset([
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c",
  ])

  ydb_region = "pre-prod_global"
}

data "yandex_vpc_network" "service_network" {
  name      = local.network_name
  folder_id = local.billing_service_folder
}

data "yandex_vpc_subnet" "service_subnets" {
  for_each = local.zones

  folder_id = local.billing_service_folder
  name      = "billing-nets-${each.key}"
}

resource "yandex_ydb_database_dedicated" "uniq" {
  name      = "billing-uniq"
  folder_id = local.billing_service_folder

  network_id = data.yandex_vpc_network.service_network.id
  subnet_ids = [
    for k, v in data.yandex_vpc_subnet.service_subnets : v.id
  ]

  resource_preset_id = "medium"

  scale_policy {
    fixed_scale {
      size = 2
    }
  }

  storage_config {
    group_count     = 2
    storage_type_id = "ssd"
  }

  location {
    region {
      id = local.ydb_region
    }
  }

  labels = {
    ipv6-network-enable = "1"
  }
}

output "ydb-uniq" {
  value = {
    "id"                = yandex_ydb_database_dedicated.uniq.id
    "name"              = yandex_ydb_database_dedicated.uniq.name
    "ydb_api_endpoint"  = yandex_ydb_database_dedicated.uniq.ydb_api_endpoint
    "ydb_full_endpoint" = yandex_ydb_database_dedicated.uniq.ydb_full_endpoint
  }
}

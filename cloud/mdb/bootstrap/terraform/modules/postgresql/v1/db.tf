resource "yandex_mdb_postgresql_cluster" "database" {
  labels = {
    mdb-auto-purge : "off"
  }
  environment = "PRODUCTION"
  name        = var.db_name
  network_id  = var.network_id
  security_group_ids = [
    yandex_vpc_security_group.postgres_sg.id,
    yandex_vpc_security_group.dns_works.id,
  ]
  config {
    version = 13
    resources {
      disk_type_id       = var.resources.disk_type_id
      disk_size          = var.resources.disk_size
      resource_preset_id = var.resources.resource_preset_id
    }
    postgresql_config = {
      max_connections = 400
    }
  }

  database {
    name  = var.db_name
    owner = var.database.owner_name
    dynamic "extension" {
      for_each = var.database.extensions
      content {
        name = extension.value
      }
    }
  }
  dynamic "host" {
    for_each = var.hosts_placement
    content {
      subnet_id = host.value["subnet_id"]
      zone      = host.value["zone"]
    }
  }
  user {
    name     = var.database.owner_name
    password = var.database.owner_password
  }

  dynamic "user" {
    for_each = var.users
    content {
      name       = user.value.name
      password   = user.value.password
      login      = user.value.login
      conn_limit = user.value.conn_limit
      permission {
        database_name = var.db_name
      }
    }
  }

}

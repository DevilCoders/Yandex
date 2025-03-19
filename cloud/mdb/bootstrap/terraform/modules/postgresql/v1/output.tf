output "connection" {
  value = {
    hosts   = yandex_mdb_postgresql_cluster.database.host[*].fqdn
    port    = 6432
    name    = var.db_name
    primary = "c-${yandex_mdb_postgresql_cluster.database.id}.rw.${var.fqdn_suffix}"
  }
}

output "database" {
  value = var.database
}

output "users" {
  value     = var.users
  sensitive = true
}

variable "DB_ADMIN_PASSWORD" {
  type      = string
  sensitive = true
}

resource "yandex_mdb_postgresql_cluster" "cm-pg-cluster" {
  deletion_protection = false
  environment         = "PRODUCTION"
  name                = "cm-pg-cluster"
  network_id          = yandex_vpc_network.cm-net.id

  labels = {
    "mdb-auto-purge" = "off"
  }

  config {
    autofailover              = true
    backup_retain_period_days = 7
    postgresql_config         = {}
    version                   = "13"

    access {
      data_lens = false
      web_sql   = false
    }

    backup_window_start {
      hours   = 22
      minutes = 0
    }

    performance_diagnostics {
      enabled                      = false
      sessions_sampling_interval   = 1
      statements_sampling_interval = 60
    }

    resources {
      disk_size          = 20
      disk_type_id       = "network-hdd"
      resource_preset_id = "b2.medium"
    }
  }

  database {
    lc_collate = "C"
    lc_type    = "C"
    name       = "cm-db"
    owner      = "cm-admin"
  }

  host {
    name             = "ru-central1-a-host1"
    assign_public_ip = false
    priority         = 0
    subnet_id        = ycp_vpc_subnet.cm-subnet["ru-central1-a"].id
    zone             = "ru-central1-a"
  }
  host {
    name                    = "ru-central1-b-host1"
    replication_source_name = "ru-central1-a-host1"
    assign_public_ip        = false
    priority                = 0
    subnet_id               = ycp_vpc_subnet.cm-subnet["ru-central1-b"].id
    zone                    = "ru-central1-b"
  }
  host {
    name                    = "ru-central1-c-host1"
    replication_source_name = "ru-central1-a-host1"
    assign_public_ip        = false
    priority                = 0
    subnet_id               = ycp_vpc_subnet.cm-subnet["ru-central1-c"].id
    zone                    = "ru-central1-c"
  }

  maintenance_window {
    type = "ANYTIME"
  }

  user {
    conn_limit = 50
    login      = true
    name       = "cm-admin"
    password   = var.DB_ADMIN_PASSWORD
    settings   = {}

    permission {
      database_name = "cm-db"
    }
  }
}

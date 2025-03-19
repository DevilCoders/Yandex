data "external" "database_secrets" {
  program = [
    "ya", "vault", "get", "version", "-o", "-j", var.yav_database_secret_id
  ]
}

resource "yandex_vpc_security_group" "database" {
  folder_id = var.folder_id

  network_id = ycp_vpc_network.mr_prober_control.id
  name = "database-sg"
  description = "Security group for PostgreSQL database"

  ingress {
    protocol = "TCP"
    description = "Allow PostgreSQL connections"
    v4_cidr_blocks = values(var.control_network_ipv4_cidrs)
    v6_cidr_blocks = values(var.control_network_ipv6_cidrs)
    port = 6432
  }
}

resource "yandex_mdb_postgresql_cluster" "database" {
  name = "mr_prober"
  environment = "PRODUCTION"

  folder_id = var.folder_id

  network_id = ycp_vpc_network.mr_prober_control.id

  config {
    version = 13

    resources {
      resource_preset_id = var.database_resource_preset
      disk_type_id       = "network-ssd"
      disk_size          = 16
    }

    access {
      web_sql = true
    }
  }

  labels = {
    layer = "iaas"
    abc_svc = "ycvpc"
    env = var.environment

    mdb-auto-purge = "off"
  }

  database {
    name  = "mr_prober"
    owner = "mr_prober"
  }

  user {
    name     = "mr_prober"
    password = data.external.database_secrets.result["${var.mr_prober_environment}_password"]
    permission {
      database_name = "mr_prober"
    }
    conn_limit = 300
  }

  dynamic host {
    for_each = var.database_zones
    content {
      zone = host.value
      subnet_id = lookup(local.control_network_subnet_ids, host.value)
    }
  }

  security_group_ids = [
    yandex_vpc_security_group.database.id
  ]

  lifecycle {
    prevent_destroy = true
  }
}

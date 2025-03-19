
data "external" "mongo_passwords" {
  program = [
    "ya", "vault", "get", "version", "-o", "-j", var.yav_mongo_password
  ]
}

resource "yandex_mdb_mongodb_cluster" "monops" {
  name        = "monops"
  environment = "PRESTABLE"
  folder_id = var.folder_id
  network_id  = var.monops_network_id

  labels = {
    layer = "iaas"
    abc_svc = "ycvpc"
    env = var.environment

    mdb-auto-purge = "off"
  }

  cluster_config {
    version = "4.2"
  }

  database {
    name = "monops"
  }

  user {
    name     = "monops"
    password = data.external.mongo_passwords.result[var.ycp_profile]
    permission {
      database_name = "monops"
      roles = [
        "readWrite"
      ]
    }
  }

  resources {
    resource_preset_id = (length(var.monops_zones) == 1) ? "b1.nano" : "b2.medium"
    disk_size          = 16
    disk_type_id       = "network-hdd"
  }

  dynamic host {
    for_each = var.monops_zones
    content {
      zone_id = host.value
      subnet_id = var.monops_network_subnet_ids[host.value]
    }
  }
}


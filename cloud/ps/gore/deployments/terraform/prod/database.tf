resource "yandex_mdb_postgresql_cluster" "dutybot-pgaas" {
  name        = "dutybot-prod"
  environment = "PRODUCTION"
  network_id  = module.network.network_id
  deletion_protection = true

  config {
    version = 12
    resources {
      resource_preset_id = "s2.micro"
      disk_type_id       = "network-ssd"
      disk_size          = 16
    }
  }

  database {
    name  = "cloudbot"
    owner = "dumb_machine"
  }

  user {
    name     = "dumb_machine"
    password = "TXB9mgCfj1OWjlcF"
    permission {
      database_name = "cloudbot"
    }
  }

  user {
    name     = "andgein"
    password = ""
    permission {
      database_name = "cloudbot"
    }
  }

  user {
    name     = "bender"
    password = ""
    permission {
      database_name = "cloudbot"
    }
  }

  host {
    zone             = "ru-central1-a"
    subnet_id        = module.network.subnet_ids["ru-central1-a"]
    assign_public_ip = true
  }

  host {
    zone             = "ru-central1-b"
    subnet_id        = module.network.subnet_ids["ru-central1-b"]
    assign_public_ip = true
  }

  host {
    zone             = "ru-central1-c"
    subnet_id        = module.network.subnet_ids["ru-central1-c"]
    assign_public_ip = true
  }
}

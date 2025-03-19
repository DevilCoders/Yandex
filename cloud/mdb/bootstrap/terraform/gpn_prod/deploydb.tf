module "deploydb" {
  source = "../modules/instance/v1"

  installation       = local.installation
  service_name       = "deploydb"
  instances_per_zone = 3

  resources = {
    core_fraction = 50
    cores         = 2
    memory        = 8
  }

  boot_disk_spec = {
    image_id = "d8ocvca3ul3o1n0lqs13"
    size     = 45
    type_id  = "network-hdd"
  }
}

resource "ycp_storage_bucket" "deploydb" {
  bucket = "mdb-backup-deploydb"
}

resource "ycp_compute_placement_group" "mdb-deploydb_pg" {
  name = "mdb-deploydb-pg"
  spread_placement_strategy {
    best_effort            = false
    max_instances_per_node = 1
  }
}

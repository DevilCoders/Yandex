module "salt" {
  source = "../modules/instance/v1"

  installation       = local.installation
  service_name       = "salt"
  instances_per_zone = 3

  resources = {
    core_fraction = 50
    cores         = 2
    memory        = 8
  }

  boot_disk_spec = {
    image_id = "d8ocvca3ul3o1n0lqs13"
    size     = 20
    type_id  = "network-hdd"
  }
}

resource "ycp_iam_service_account" "salt-master" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "salt-master"
  service_account_id = "yc.mdb.salt-master"
}

resource "ycp_iam_service_account" "salt-sync" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "salt-sync"
  service_account_id = "yc.mdb.salt-sync"
}

resource "ycp_storage_bucket" "mdb-salt-images" {
  bucket = "mdb-salt-images"
}

resource "ycp_compute_placement_group" "mdb-salt_pg" {
  name = "mdb-salt-pg"
  spread_placement_strategy {
    best_effort            = false
    max_instances_per_node = 1
  }
}

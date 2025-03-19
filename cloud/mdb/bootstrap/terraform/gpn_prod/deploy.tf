module "deploy" {
  source = "../modules/instance/v1"

  installation       = local.installation
  service_name       = "deploy"
  instances_per_zone = 2

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

  create_load_balancer = true
}

resource "ycp_compute_placement_group" "mdb-deploy_pg" {
  name = "mdb-deploy-pg"
  spread_placement_strategy {
    best_effort            = false
    max_instances_per_node = 1
  }
}

module "zk01" {
  source = "../modules/cluster/v1"
  name   = "zk01"

  resources = {
    core_fraction = 50
    cores         = 2
    memory        = 8
  }

  installation = var.installation

  boot_disk = {
    image_id = var.image_id
    type_id  = "network-ssd"
    size     = 30
  }
}

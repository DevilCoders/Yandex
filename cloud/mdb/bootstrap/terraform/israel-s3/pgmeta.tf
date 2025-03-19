module "pgmeta01" {
  source = "../modules/cluster/v1"
  name   = "pgmeta01"

  resources = {
    core_fraction = 50
    cores         = 2
    memory        = 8
  }
  platform_id = "standard-v3"

  installation        = var.installation
  node_count_per_zone = 2

  boot_disk = {
    image_id = var.image_id
    type_id  = "network-ssd"
    size     = 50
  }
  can_get_all_secrets_in_folder = true
}

module "s3db01" {
  source = "../modules/cluster/v1"
  name   = "s3db01"

  resources = {
    core_fraction = 100
    cores         = 4
    memory        = 16
  }
  platform_id = "standard-v3"

  installation = var.installation

  boot_disk = {
    image_id = var.image_id
    type_id  = "network-ssd"
    size     = 300
  }
  can_get_all_secrets_in_folder = true
}

module "samba" {
  source       = "../modules/instance/v1"
  installation = local.installation
  service_name = "samba"

  resources = {
    core_fraction = 100
    cores         = 4
    memory        = 8
  }

  boot_disk_spec = {
    size     = 50
    image_id = "fdvl7uah5p552o24s7rm"
    type_id  = "network-ssd"
  }
}

module "deploy" {
  source = "../modules/instance/v1"

  installation                        = local.installation
  service_name                        = "deploy"
  override_shortname_prefix           = "mdb-deploy-api"
  override_zone_shortname_with_letter = true

  resources = {
    core_fraction = 50
    cores         = 4
    memory        = 8
  }

  boot_disk_spec = {
    size     = 30
    image_id = "fd813h5asrmm6lel0mu7"
    type_id  = "network-hdd"
  }

  override_load_balancer_ipv6_addr = "2a0d:d6c0:0:1b::3b5"
  create_load_balancer             = true
}

module "deploydb" {
  source = "../modules/instance/v1"

  installation                        = local.installation
  service_name                        = "deploydb"
  override_shortname_prefix           = "mdb-deploy-db"
  override_zone_shortname_with_letter = true

  resources = {
    core_fraction = 100
    cores         = 8
    memory        = 32
  }

  boot_disk_spec = {
    size     = 96
    image_id = "fd813h5asrmm6lel0mu7"
    type_id  = "network-hdd"
  }
}

resource "yandex_iam_service_account" "salt-images" {
  name        = "salt-images"
  description = "MDB-7115 service account to manage salt-images"
}

resource "ycp_iam_service_account" "salt-images-reader" {
  name               = "salt-images-reader"
  description        = "MDB-11531 service account to download salt images"
  service_account_id = "yc.mdb.salt-images-reader"
}

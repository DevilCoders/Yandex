module "salt-common" {
  source = "../modules/instance/v1"

  installation                                = local.installation
  service_name                                = "salt-common"
  override_shortname_prefix                   = "mdb-deploy-salt-common"
  override_instance_env_suffix_with_emptiness = true
  override_zone_shortname_with_letter         = true

  resources = {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk_spec = {
    size     = 30
    image_id = "fd813h5asrmm6lel0mu7"
    type_id  = "network-hdd"
  }
  override_boot_disks = {
    "01.zone_a" = data.yandex_compute_disk.mdb-deploy-salt-common-preprod01k_boot
    "01.zone_b" = data.yandex_compute_disk.mdb-deploy-salt-common-preprod01h_boot
    "01.zone_c" = data.yandex_compute_disk.mdb-deploy-salt-common-preprod01f_boot
  }

  disable_seccomp = true
}

data "yandex_compute_disk" "mdb-deploy-salt-common-preprod01f_boot" {
  disk_id = "d9hu32ll8085jc3uq3dl"
}

data "yandex_compute_disk" "mdb-deploy-salt-common-preprod01h_boot" {
  disk_id = "c8rnufv2ttphdh2uqq12"
}
data "yandex_compute_disk" "mdb-deploy-salt-common-preprod01k_boot" {
  disk_id = "a7lrppn4r94qsmjc5ssl"
}

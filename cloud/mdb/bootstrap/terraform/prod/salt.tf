module "salt" {
  source = "../modules/instance/v1"

  installation                        = local.installation
  service_name                        = "salt"
  override_shortname_prefix           = "mdb-deploy-salt"
  override_zone_shortname_with_letter = true
  instances_per_zone                  = 4
  allow_stopping_for_update           = false

  resources = {
    core_fraction = 100
    cores         = 32
    memory        = 32
  }

  boot_disk_spec = {
    size     = 100
    image_id = "fd89mjc163odvsb5goki"
    type_id  = "network-ssd"
  }
  override_boot_disks = {
    "01.zone_a" = {
      size     = 100
      image_id = "fd8idchv3h60mt262tt8"
      type     = "network-ssd"
    }
    "01.zone_b" = {
      auto_delete = false
      size        = 99
      image_id    = "fd8resf4lj9784nlld51"
      type        = "network-hdd"
    }
    "01.zone_c" = {
      size     = 100
      image_id = "fd8idchv3h60mt262tt8"
      type     = "network-ssd"
    }
    "02.zone_a" = {
      size     = 100
      image_id = "fd80g5uaqafefv9t67a9"
      type     = "network-hdd"
    }
    "02.zone_b" = {
      size     = 100
      image_id = "fd80g5uaqafefv9t67a9"
      type     = "network-hdd"
    }
    "02.zone_c" = {
      size     = 100
      image_id = "fd80g5uaqafefv9t67a9"
      type     = "network-hdd"
    }
    "03.zone_a" = {
      size     = 100
      image_id = "fd83i6sk53bcdrnl012d"
      type     = "network-ssd"
    }
    "03.zone_b" = {
      size     = 100
      image_id = "fd83i6sk53bcdrnl012d"
      type     = "network-ssd"
    }
    "03.zone_c" = {
      size     = 100
      image_id = "fd83i6sk53bcdrnl012d"
      type     = "network-ssd"
    }
  }
}

resource "ycp_iam_service_account" "salt-master" {
  name               = "salt-master"
  description        = "MDB-11346 service account for salt-master"
  service_account_id = "yc.mdb.salt-master"
}

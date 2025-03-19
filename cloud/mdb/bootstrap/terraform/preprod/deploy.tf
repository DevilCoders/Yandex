module "deploydb" {
  source = "../modules/instance/v1"

  installation                        = local.installation
  service_name                        = "deploydb"
  override_shortname_prefix           = "mdb-deploy-db"
  override_zone_shortname_with_letter = true

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
    "01.zone_a" = data.yandex_compute_disk.mdb-deploy-db-preprod01k_boot
    "01.zone_b" = {
      size     = 30
      image_id = "fdv19ubn2bdfq62v5mop"
      type_id  = "network-hdd"
    }
    "01.zone_c" = data.yandex_compute_disk.mdb-deploy-db-preprod01f_boot
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

data "yandex_compute_disk" "mdb-deploy-db-preprod01k_boot" {
  disk_id = "a7lojmfbhvul1eam34a6"
}

data "yandex_compute_disk" "mdb-deploy-db-preprod01f_boot" {
  disk_id = "d9herv6l27kqun16q40l"
}

module "salt" {
  source = "../modules/instance/v1"

  installation                        = local.installation
  service_name                        = "salt"
  override_shortname_prefix           = "mdb-deploy-salt"
  override_zone_shortname_with_letter = true

  resources = {
    core_fraction = 100
    cores         = 4
    memory        = 16
  }

  boot_disk_spec = {
    size     = 30
    image_id = "fd813h5asrmm6lel0mu7"
    type_id  = "network-hdd"
  }
  override_boot_disks = {
    "01.zone_a" = data.yandex_compute_disk.mdb-deploy-salt-preprod01k_boot
    "01.zone_b" = {
      size     = 30
      image_id = "fdv19ubn2bdfq62v5mop"
      type_id  = "network-hdd"
    }
    "01.zone_c" = data.yandex_compute_disk.mdb-deploy-salt-preprod01f_boot
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

data "yandex_compute_disk" "mdb-deploy-salt-preprod01k_boot" {
  disk_id = "a7lh65l1bhujrs7qtp4h"
}

data "yandex_compute_disk" "mdb-deploy-salt-preprod01f_boot" {
  disk_id = "d9hn65b69q470b3fh386"
}

resource "ycp_iam_service_account" "salt-images-reader" {
  name               = "salt-images-reader"
  description        = "MDB-11531 service account to download salt images"
  service_account_id = "yc.mdb.salt-images-reader"
}

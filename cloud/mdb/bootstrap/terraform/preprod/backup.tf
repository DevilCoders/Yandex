module "backup" {
  source = "../modules/instance/v1"

  installation = local.installation
  service_name = "backup"

  resources = {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk_spec = {
    size     = 30
    image_id = "fdvlmfdtdt7narikgh2e"
    type_id  = "network-hdd"
  }

  override_boot_disks = {
    "01.zone_b" = {
      size     = 30
      image_id = "fdv2sbduod8qobb2vm47"
      type_id  = "network-hdd"
    }
  }
}

resource "ycp_iam_service_account" "mdb-backup-worker" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-backup-worker"
  description        = "Manages backups for data-plane clusters"
  service_account_id = "yc.mdb.backup-worker"
}

module "backups" {
  source   = "../modules/backups/v1"
  cloud_id = var.cloud_id
}
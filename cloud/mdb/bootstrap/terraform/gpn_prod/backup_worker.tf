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
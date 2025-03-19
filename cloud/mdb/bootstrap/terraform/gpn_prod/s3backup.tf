resource "ycp_iam_service_account" "s3backup" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "s3backup"
  description        = "backups for control plane clusters"
  service_account_id = "yc.mdb.s3backup"
}

resource "ycp_iam_service_account" "s3backup-clusters" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "s3backup-clusters"
  description        = "backups for data-plane clusters"
  service_account_id = "yc.mdb.s3backup-clusters"
}

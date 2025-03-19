resource "ycp_iam_service_account" "mdb-maintenance" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-maintenance"
  description        = "Performs maintenance tasks"
  service_account_id = "yc.mdb.maintenance"
}

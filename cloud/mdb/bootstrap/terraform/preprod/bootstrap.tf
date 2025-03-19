resource "ycp_iam_service_account" "mdb-bootstrap" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-bootstrap"
  description        = "Account to apply terraform configuration"
  service_account_id = "yc.mdb.bootstrap"
}

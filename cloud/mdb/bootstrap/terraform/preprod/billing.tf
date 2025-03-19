resource "ycp_iam_service_account" "mdb-billing" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-billing"
  description        = "MDB-10978"
  service_account_id = "yc.mdb.billing"
}

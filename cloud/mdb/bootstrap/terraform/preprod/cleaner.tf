resource "ycp_iam_service_account" "mdb-cleaner" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-cleaner"
  description        = "Purges resources in non-production environments"
  service_account_id = "yc.mdb.cleaner"
}

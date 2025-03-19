resource "ycp_iam_service_account" "dataproc-manager" {
  name               = "dataproc-manager"
  service_account_id = "yc.dataproc.manager"
  description        = "MDB-17066"
}

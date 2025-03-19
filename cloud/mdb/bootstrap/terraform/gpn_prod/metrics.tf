resource "ycp_iam_service_account" "metrics-controlplane" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "metrics-controlplane"
  description        = "Sends metrics to mdb service cloud"
  service_account_id = "yc.mdb.metrics-controlplane"
}

resource "ycp_iam_service_account" "metrics-dataplane" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "metrics-dataplane"
  description        = "Sends metrics to users' clouds"
  service_account_id = "yc.mdb.metrics-dataplane"
}

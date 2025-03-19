resource "ycp_iam_service_account" "metrics-controlplane" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "metrics-controlplane"
  description        = "Sends metrics to mdb service cloud"
  service_account_id = "bfb23d4tss48qn1eena6"
}

resource "ycp_iam_service_account" "metrics-dataplane" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "metrics-dataplane"
  description        = "Sends metrics to users' clouds"
  service_account_id = "bfbn7sft5hn8ts6016lb"
}

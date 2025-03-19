resource "ycp_iam_service_account" "logs-dataplane-producer" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "logs-dataplane-producer"
  service_account_id = "yc.mdb.logs_dataplane_producer"
  description        = "SA for log shipment. See MDB-11040"
}

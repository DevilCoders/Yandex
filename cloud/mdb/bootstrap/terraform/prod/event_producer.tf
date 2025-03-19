resource "ycp_iam_service_account" "mdb-event-producer" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-event-producer"
  description        = "Sends Cloud.Events"
  service_account_id = "yc.mdb.event-producer"
}

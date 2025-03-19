resource "yandex_iam_service_account" "s3-editor" {
  name = "s3-editor"
}

resource "yandex_iam_service_account" "tf-s3-admin" {
  name        = "s3-admin"
  description = "services account for buckets management"
}

resource "yandex_iam_service_account" "robot-mon-terraform-plan" {
  name        = "robot-mon-terraform-plan"
  description = "MDB-8466 monitors preprod terraform plan"
}

resource "yandex_iam_service_account" "ydpagent" {
  name        = "ydpagent"
  description = "MDB-3863: with role internal.ydpagent. global SA for hadoop"
}

resource "yandex_iam_service_account" "salt-images" {
  name        = "salt-images"
  description = "MDB-7115 service account to manage salt-images"
}

resource "yandex_iam_service_account" "salt-master" {
  name        = "salt-master"
  description = "MDB-9110"
}

resource "ycp_iam_service_account" "logs-dataplane-producer" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "logs-dataplane-producer"
  service_account_id = "yc.mdb.logs_dataplane_producer"
  description        = "SA for log shipment. See MDB-11040"
}

resource "ycp_iam_service_account" "mdb_on_call" {
  name               = "mdb-on-call"
  description        = "MDB-9406: service account for manual support operations with user resources"
  service_account_id = "yc.mdb.oncall"
}

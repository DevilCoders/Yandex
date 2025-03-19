resource "yandex_storage_bucket" "mdb-salt-images" {
  bucket = "mdb-salt-images"
  grant {
    id          = ycp_iam_service_account.salt-images-reader.id
    permissions = ["READ"]
    type        = "CanonicalUser"
  }
}

resource "yandex_storage_bucket" "mdb-prod-control-plane-tfstate" {
  bucket = "mdb-prod-control-plane-tfstate"
  acl    = "private"
}

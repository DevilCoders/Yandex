# Sadly, but current ycp provider, can't create buckets with `acl`.
# As workaround I:
# - create bucket with tf
# - drop bucket in UI and recreate it with public access
resource "ycp_storage_bucket" "errata" {
  bucket = "errata"
}

resource "ycp_iam_service_account" "errata-uploader" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "errata-uploader"
  description        = "account to upload errata updates"
  service_account_id = "yc.mdb.errata-uploader"
}

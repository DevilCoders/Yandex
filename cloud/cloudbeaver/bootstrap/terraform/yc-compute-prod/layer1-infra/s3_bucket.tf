resource "yandex_kms_symmetric_key" "cloudbeaver-s3fs" {
  name              = "cloudbeaver-s3fs"
  description       = "key for sever-side s3 bucket encryption"
  default_algorithm = "AES_256"
}

resource "yandex_storage_bucket" "cloudbeaver-s3fs" {
  bucket = "cloudbeaver-s3fs"
  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        kms_master_key_id = yandex_kms_symmetric_key.cloudbeaver-s3fs.id
        sse_algorithm     = "aws:kms"
      }
    }
  }
}

terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "israel-kms-terraform-state"
    key        = "kms-ydb-dumper"
    region     = "us-east-1"
    access_key = "BXG7EwtzGyyx0GTNPPm_"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

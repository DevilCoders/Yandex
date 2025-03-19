terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "hw-blue-lab-kms-state"
    key        = "kms-data-plane"
    region     = "us-east-1"
    access_key = "nubNZMn45O377n4MXu8q"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

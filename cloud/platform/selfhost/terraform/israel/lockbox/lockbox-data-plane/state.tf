terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "israel-lockbox-terraform-state"
    key        = "lockbox-data-plane"
    region     = "us-east-1"
    access_key = "ZWteRXe6ySGqpkfmibEV"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

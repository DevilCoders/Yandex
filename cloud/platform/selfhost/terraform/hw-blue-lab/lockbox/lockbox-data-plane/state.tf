terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "hw-blue-lab-lockbox-state"
    key        = "lockbox-data-plane"
    region     = "us-east-1"
    access_key = "TWGD1RmYwcgd-TOOT6bH"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

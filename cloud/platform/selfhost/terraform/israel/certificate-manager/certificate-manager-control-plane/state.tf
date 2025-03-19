terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "israel-ycm-terraform-state"
    key        = "cpl"
    region     = "us-east-1"
    access_key = "tNOuQd7pccuP6WEO9yqW"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

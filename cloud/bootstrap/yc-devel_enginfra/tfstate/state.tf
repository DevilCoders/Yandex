terraform {
  backend "s3" {
    endpoint   = "https://storage.yandexcloud.net"
    bucket     = "yc-devel-enginfra-tfstate"
    key        = "tfstate"
    region     = "us-east-1"
    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

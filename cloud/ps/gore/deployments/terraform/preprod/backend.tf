terraform {
  backend "s3" {
    endpoint = "storage.yandexcloud.net"
    bucket = "gore-terraform-states"
    key = "preprod.tfstate"
    region = "us-east-1"

    skip_credentials_validation = true
    skip_metadata_api_check = true
  }
}


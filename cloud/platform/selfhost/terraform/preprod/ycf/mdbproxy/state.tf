terraform {
  backend "s3" {
    endpoint = "storage.cloud-preprod.yandex.net"
    bucket   = "serverless-infra-state"
    key      = "terraform-state/database/ycf-mdbproxy-immutable.tfstate"
    region   = "us-east-1"
    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}


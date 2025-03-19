terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "terraform-state"
    key        = "preprod/api-adapter.tfstate"
    region     = "us-east-1"
    access_key = "d922_h1sdxKc4oyLWEYE"

    skip_requesting_account_id  = true
    skip_credentials_validation = true
    skip_get_ec2_platforms      = true
    skip_metadata_api_check     = true
  }
}

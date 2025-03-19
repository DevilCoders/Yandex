terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "hw-blue-lab-certificate-manager-state"
    key        = "dpl"
    region     = "us-east-1"
    access_key = "NCcV4lOjTawnDKBMJQhp"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

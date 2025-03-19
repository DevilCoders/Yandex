terraform {
  backend "s3" {
    endpoint   = "s3.mds.yandex.net"
    bucket     = "yc-bootstrap"
    key        = "terraform/build-agents"
    access_key = "S8KusGXPOGJGlniTdLdm"
    // secret_key must set from backend_config arg
    region = "us-east-1"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

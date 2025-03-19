terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "lockbox-load-tester-state"
    key        = "state"
    region     = "us-east-1"

    access_key = "Qo1Rl38FKVv2TqyejowW"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

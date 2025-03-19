terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "terraform-state-schecker"
    key        = "prod/schecker-devel"
    region     = "us-east-1"
    // Access is done via the sa-terraform-s3 service account from the yc-schecker cloud.
    // secret_key is stored in lockbox (https://console.cloud.yandex.ru/folders/yc.schecker/lockbox/secret/e6qectt4bg15nb2256me)
    // and must be passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "LeazWbFdtVYR4YV8AIEb"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

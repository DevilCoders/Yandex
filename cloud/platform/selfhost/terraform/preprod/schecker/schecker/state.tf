terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "terraform-state-schecker"
    key        = "preprod/schecker-devel"
    region     = "us-east-1"
    // Access is done via the sa-terraform-s3 service account from the yc-schecker cloud.
    // secret_key is stored in lockbox (https://console-preprod.cloud.yandex.ru/folders/yc.schecker/lockbox/secret/fc310n0a5stunac2vd22)
    // and must be passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "f4Ecu-6c_36l9yBPds_o"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

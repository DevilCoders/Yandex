terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "lockbox-data-plane-terraform-state"
    key        = "state"
    region     = "us-east-1"
    // Access is done via 'terraform' service account from 'yc-lockbox' cloud.
    // Secret_key is stored in yav.yandex-team.ru (yc-lockbox-preprod-terraform-service-account) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "ZeVFEZo93FwgY0pQ_CdL"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

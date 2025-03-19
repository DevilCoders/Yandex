terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "lockbox-ydb-dumper-terraform-state"
    key        = "state"
    region     = "us-east-1"
    // Access is done via 'terraform-account' service account from 'yc-lockbox' cloud.
    // Secret_key is stored in yav.yandex-team.ru (yc-lockbox-preprod-backup-terraform-service-account) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "aREArmpVS4lMZB9D_ZWW"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

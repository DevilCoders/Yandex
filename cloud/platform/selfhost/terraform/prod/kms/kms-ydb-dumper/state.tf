terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "kms-ydb-dumper-terraform-state"
    key        = "prod/state"
    region     = "us-east-1"
    // Access is done via the sa-terraform-deploy service account from the yc-kms cloud.
    // secret_key is stored in yav.yandex-team.ru (yckms-prod-backup-sa-terraform-deploy) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "t3dpZllnmj9iINeq3J71"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

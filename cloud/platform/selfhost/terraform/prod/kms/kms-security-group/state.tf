terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "kms-security-group-terraform-state"
    key        = "prod/kms"
    region     = "us-east-1"
    // Access is done via the sa-terraform-s3 service account from the yc-kms cloud.
    // secret_key is stored in yav.yandex-team.ru (yc-preprod-kms-sa-terraform) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "NNDG0H5X4oxUrZzUaRIw"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "kms-monitoring-terraform-state"
    key        = "preprod/kms"
    region     = "us-east-1"
    // Access is done via the sa-monitoring-terraform-s3 service account from the yc-kms cloud.
    // secret_key is stored in yav.yandex-team.ru (yc-preprod-kms-sa-monitoring-terraform-s3-secret) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "op1K_K1_6VaNRfovZFMS"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

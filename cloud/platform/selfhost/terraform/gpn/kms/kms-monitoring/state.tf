terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "kms-monitoring-terraform-state-private-gpn"
    key        = "private-gpn/kms"
    region     = "us-east-1"
    // Access is done via the sa-monitoring-terraform-s3 service account from the yc-kms cloud.
    // secret_key is stored in yav.yandex-team.ru (sa-monitoring-terraform-s3-secret) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "1fj7ztJJzbq2eB_jsgQL"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

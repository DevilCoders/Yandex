terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "testing-kms-control-plane-terraform-state"
    key        = "testing/kms"
    region     = "us-east-1"
    // Access is done via the sa-terraform-testing-s3 service account from the yc-kms-devel cloud.
    // secret_key is stored in yav.yandex-team.ru (yc-preprod-kms-sa-terraform-testing) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "Wnjt1vfjEACtcKLvZH3N"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

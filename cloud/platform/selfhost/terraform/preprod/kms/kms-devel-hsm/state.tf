terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "kms-devel-terraform-state"
    key        = "preprod/kms-devel-hsm"
    region     = "us-east-1"
    // Access is done via the sa-terraform service account from the yc-kms-devel cloud.
    // secret_key is stored in yav.yandex-team.ru (yc-preprod-kms-devel-hsm-sa-terraform) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "SlPqVLYBEPEYFaSh8KB_"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

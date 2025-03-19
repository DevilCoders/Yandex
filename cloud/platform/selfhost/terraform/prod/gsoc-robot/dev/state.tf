terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "yc-gsoc-robot-terraform-state"
    key        = "prod/gsoc-robot"
    region     = "us-east-1"
    // Access is done via the sa-terraform-s3 service account from the yc-osquery cloud.
    // secret_key is stored in yav.yandex-team.ru (yc-prod-osquery-sa-terraform-secret) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "L-qUmGee1A1yq8-OghWa"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

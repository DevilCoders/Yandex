terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "yc-osquery-sender-terraform-state"
    key        = "preprod/osquery-sender"
    region     = "us-east-1"
    // Access is done via the sa-terraform-s3 service account from the yc-osquery cloud.
    // secret_key is stored in yav.yandex-team.ru (yc-preprod-osquery-sa-terraform-secret) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "aC5H45JjwIU_SwMPhlnF"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

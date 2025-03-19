terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "trail-terraform-state"
    key        = "preprod/trail-tool"
    region     = "us-east-1"
    // Access is done via the sa-terraform service account from the yc-cloud-trail cloud.
    // secret_key is stored in yav.yandex-team.ru (cloud-trail-preprod-sa-terraform-token) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "qsmCa5njB-mARQ1SYPVl"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

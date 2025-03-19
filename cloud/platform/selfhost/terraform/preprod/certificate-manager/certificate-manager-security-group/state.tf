terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "certificate-manager-security-group-terraform-state"
    key        = "preprod/certificate-manager-devel"
    region     = "us-east-1"
    // Access is done via the sa-terraform service account from the yc-kms-devel cloud.
    // secret_key is stored in yav.yandex-team.ru (ycloud-ycm-preprod-sa-terraform-token) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "9bgot6Y7hHjgQlUQyxbB"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "iot-tfstate"
    key        = "preprod/iot.tfstate"
    region     = "us-east-1"

    // Access is done via the terraform service account from the iot folder.
    // secret_key is stored in yav.yandex-team.ru (terraform_preprod_state) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "KmDMiDPloOY5jsq9uIbJ"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

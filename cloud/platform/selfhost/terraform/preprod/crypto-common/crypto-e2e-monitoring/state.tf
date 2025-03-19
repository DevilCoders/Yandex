terraform {
  backend "s3" {
    endpoint   = "storage.cloud-preprod.yandex.net"
    bucket     = "crypto-e2e-monitoring-tf-state"
    key        = "state"
    region     = "us-east-1"
    // Access is done via 'e2e-monitoring-terraform' service account from 'yc-crypto-e2e-monitoring' cloud.
    // Secret_key is stored in yav.yandex-team.ru (yc-crypto-e2e-preprod-tf-sa) and must be
    // passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=<secret_value_right_here>"`
    access_key = "wpHSUA9H9CfJhDYt5MKS"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

terraform {
  backend "s3" {
    endpoint = "storage.yandexcloud.net"
    bucket   = "terraform-state-nfs"
    key      = "prod/filestore"
    region   = "us-east-1"

    access_key = "QVe8kxzT18O3BcoXjjEv"
    // secret_key = ""
    // should be specified to "terraform init" using command line:
    // -backend-config="secret_key=<secret_value_right_here>"

    skip_requesting_account_id  = true
    skip_credentials_validation = true
    skip_get_ec2_platforms      = true
    skip_metadata_api_check     = true
  }
}

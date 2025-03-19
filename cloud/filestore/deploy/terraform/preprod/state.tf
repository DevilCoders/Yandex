terraform {
  backend "s3" {
    endpoint = "storage.cloud-preprod.yandex.net"
    bucket   = "terraform-state-nfs"
    key      = "preprod/filestore"
    region   = "us-east-1"

    access_key = "RwdLNB7Hmvb6Ap_rTohD"
    // secret_key = ""
    // should be specified to "terraform init" using command line:
    // -backend-config="secret_key=<secret_value_right_here>"

    skip_requesting_account_id  = true
    skip_credentials_validation = true
    skip_get_ec2_platforms      = true
    skip_metadata_api_check     = true
  }
}

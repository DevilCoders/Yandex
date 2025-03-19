terraform {
    backend "s3" {
        endpoint   = "storage.cloud-preprod.yandex.net"
        bucket     = "l7-als"
        key        = "l7-als.tfstate"
        region     = "us-east-1"
        access_key = "Y9h6QtxlVsOP5SxB0w6C"

        // secret_key = "{}"   <- use command line arg
        // like ` -backend-config="secret_key=<secret_value_right_here>" `
        // for "terraform init" call

        skip_requesting_account_id  = true
        skip_credentials_validation = true
        skip_get_ec2_platforms      = true
        skip_metadata_api_check     = true
    }
}

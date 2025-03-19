terraform {
  backend "s3" {
    endpoint                    = "storage.yandexcloud.net"
    bucket                      = "serverless-infra"
    key                         = "terraform-state/ycf-immutable.tfstate"
    region                      = "us-east-1"
    // access_key = "{}"   <- use command line arg
    // secret_key = "{}"   <- use command line arg
    // like ` -backend-config="secret_key=<secret_value_right_here>" `
    // for "terraform init" call
    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}


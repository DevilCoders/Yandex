locals {
  infra = {
    name = "preprod"
  }

  app              = "openid-server"
  provider         = "yc"
}

terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "datacloud-iam-yc-preprod-tfstate"
    key        = "app-openid-server"
    region     = "us-east-1"
    // Access key for tf-state-editor SA (aje0v6tnutbl7dd03k4d) access is used.
    // secret_key is stored in yav.yandex-team.ru and must be passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=$(ya vault get version sec-01fhkejpcbm3gyfj6c5qgxnhp4 -o secret_key)"`
    access_key = "0cimnsKzHsBLfdstpx3i"
    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}

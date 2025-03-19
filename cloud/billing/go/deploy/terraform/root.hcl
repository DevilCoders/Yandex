locals {
    module_path = path_relative_from_include()
    shared_credentials_file = abspath("${get_terragrunt_dir()}/${local.module_path}/generated.aws_creds")
    built = run_cmd("${local.module_path}/tools/build-modules.sh")
}

terraform {
  after_hook "storage_keys" {
    commands = ["terragrunt-read-config"]
    execute  = ["bash", "${local.module_path}/secrets/state/decrypt_bundle.sh"]
  }
  after_hook "api_keys" {
    commands = ["terragrunt-read-config"]
    execute  = ["bash", "${local.module_path}/secrets/api/decrypt_bundle.sh"]
  }
}

remote_state {
  disable_dependency_optimization = true
  backend = "s3"
  generate = {
    path      = "generated.state.tf"
    if_exists = "overwrite_terragrunt"
  }
  config = {
    endpoint   = "https://storage.yandexcloud.net"
    bucket     = "billing-deploy-rnd"

    key = "${path_relative_to_include()}.tfstate"
    region         = "us-east-1"
    skip_credentials_validation = true
    skip_metadata_api_check = true

    shared_credentials_file = "${local.shared_credentials_file}"

    dynamodb_endpoint           = "https://docapi.serverless.yandexcloud.net/ru-central1/yc.billing.service-cloud/etnv47vg78ois2p9o24j"
    dynamodb_table              = "terraform-locks"
  }
}

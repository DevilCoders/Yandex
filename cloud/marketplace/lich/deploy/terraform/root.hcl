terraform {
  source = "../modules/logging"
}

generate "provider" {
  path = "provider.tf"
  if_exists = "overwrite_terragrunt"
  contents = <<EOF
terraform {
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }

  required_version = ">= 0.13"
}

provider "yandex" {
  token = data.external.yc_token.result.iam_token

  endpoint = var.yc_api_endpoint
  folder_id = var.folder_id
}
EOF
}

generate "yc-creds" {
  path = "yc-creds.tf"
  if_exists = "overwrite_terragrunt"
  contents = <<EOF
data "external" "yc_token" {
  program = ["yc", "iam", "create-token", "--format", "json"]
}
EOF
}


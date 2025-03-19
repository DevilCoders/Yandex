locals {
  has_api_gateway = length(regexall("^testing/", path_relative_to_include())) == 0

  terraform_config = <<EOF
terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = "0.77.0"
    }

    ytr = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ytr"
      version = "0.4.0"
    }

    yandex = {
      source = "yandex-cloud/yandex"
      version = "0.74.0"
    }

    external = {
      source = "hashicorp/external"
      version = "2.1.0"
    }
  }
}
EOF

  ycp_provider_config = <<EOF
provider "ycp" {
  ycp_profile = var.ycp_profile
}
EOF

  ytr_provider_config = <<EOF
provider "ytr" {
  conductor_token = var.yandex_team_token
}
EOF

  yandex_provider_config = <<EOF
provider "yandex" {
  token = data.external.yc_token.result.iam_token
  endpoint = var.yc_endpoint
  storage_endpoint = var.storage_endpoint
}
EOF
}

// Each stand directory should contain stand.tfvars with per-stand defaults
// Override default terraform invocation command line
terraform {
  extra_arguments "common_vars" {
    commands = ["plan", "apply", "destroy", "import"]

    arguments = [
      "-var-file=../stand.tfvars"
    ]
  }
}

remote_state {
  backend = "s3"
  generate = {
    path      = "backend.tf"
    if_exists = "overwrite"
  }
  config = {
    endpoint = "https://storage.yandexcloud.net"
    bucket = "vpc-terraform-states"
    key = "${path_relative_to_include()}/terraform.tfstate"
    region = "us-east-1"

    skip_credentials_validation = true
    skip_metadata_api_check = true
  }
}

generate "variables" {
  path      = "variables.tf"
  if_exists = "overwrite"
  contents = <<EOF
variable "yandex_team_token" {
  type = string
  description = "Token for YaV, Conductor, ABC and Staff. Can be received here: https://oauth.yandex-team.ru/authorize?response_type=token&client_id=18cc5e70bf92483796e21aecc67589ee"
}

variable "ycp_profile" {
  type = string
  description = "Name of the ycp profile"
}

variable "yc_endpoint" {
  type = string
  description = "Endpoint for public API (for yandex provider)"
}

variable "storage_endpoint" {
  type = string
  description = "Storage endpoint for yandex_storage_* resources"
  default = "https://storage.yandexcloud.net"
}

variable "cr_endpoint" {
  type = string
  description = "Endpoint for container registry"
}

variable "hc_network_ipv6" {
  type = string
  description = "IPv6 healthcheck network from https://wiki.yandex-team.ru/cloud/devel/loadbalancing/ipv6/#securitygroupsihealthchecks"
}

variable "environment" {
  type = string
  description = "Name of cloud environment, i.e. prod or testing"
}

variable "logbroker_endpoint" {
  type = string
  description = "Container logbroker endpoint"
}

variable "logbroker_database" {
  type = string
  description = "Cloud logbroker database"
}

variable "region_id" {
  type = string
  description = "The ID of the region. I.e. ru-central1"
}

variable "availability_zones" {
  type = list(string)
  description = "Availability zone names"
}

EOF
}

generate "provider" {
  path      = "provider.tf"
  if_exists = "overwrite"
  contents = join("\n\n", concat([local.terraform_config, local.ycp_provider_config, local.ytr_provider_config], local.has_api_gateway ? [local.yandex_provider_config] : []))
}

generate "credentials" {
  path      = "credentials.tf"
  if_exists = "overwrite"
  contents = !local.has_api_gateway ? "" : <<EOF
data "external" "yc_token" {
  program = ["ycp", "--profile", var.ycp_profile, "iam", "create-token", "--format", "json"]
}
EOF
}

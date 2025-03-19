module "aws" {
    source = "../../../modules/infra/clouds/aws/v1"
}

locals {
    default_tags = {
        Infra                                = local.infra.name
        Team                                 = "IAM"
        (module.aws.tags.discounts.all.name) = module.aws.tags.discounts.all.value
    }
    aws_profile = "doublecloud-aws-preprod"
}

provider "aws" {
    profile = local.aws_profile
    region  = "eu-central-1"
    allowed_account_ids = [local.aws_account]

    default_tags {
        tags = merge(
            local.default_tags,
            {
                Region = "eu-central-1"
            }
        )
    }
}

terraform {
    required_version = "0.14.9"
    required_providers {
        aws = {
            source  = "hashicorp/aws"
            version = "3.74.0"
        }
        kubernetes = {
            source  = "hashicorp/kubernetes"
            version = "2.3.2"
        }
    }
    backend "s3" {
        profile = "doublecloud-aws-preprod"
        bucket = "datacloud-iam-preprod-tfstate"
        key    = "balancers-preprod"
        region = "eu-central-1"
    }
}

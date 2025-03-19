module "aws" {
    source = "../../../modules/infra/clouds/aws/v1"
}

locals {
    default_tags = {
        Infra                                = local.infra_name
        Team                                 = "IAM"
        (module.aws.tags.discounts.all.name) = module.aws.tags.discounts.all.value
    }
    aws_profile = "doublecloud-aws-preprod"
}

provider "aws" {
    profile = local.aws_profile
    region  = module.aws.regions.frankfurt.id
    allowed_account_ids = [local.aws_account]

    default_tags {
        tags = merge(
            local.default_tags,
            {
                Region = module.aws.regions.frankfurt.id
            }
        )
    }

    // This is necessary so that tags required for eks can be applied to the vpc without changes to the vpc wiping them out.
    // https://registry.terraform.io/providers/hashicorp/aws/latest/docs/guides/resource-tagging
    ignore_tags {
        key_prefixes = ["kubernetes.io/"]
    }
}

provider "aws" {
    profile = local.aws_profile
    region  = module.aws.regions.frankfurt.id
    allowed_account_ids = [local.aws_account]
    alias   = "frankfurt"

    default_tags {
        tags = merge(
            local.default_tags,
            {
                Region = module.aws.regions.frankfurt.id
            }
        )
    }

    // This is necessary so that tags required for eks can be applied to the vpc without changes to the vpc wiping them out.
    // https://registry.terraform.io/providers/hashicorp/aws/latest/docs/guides/resource-tagging
    ignore_tags {
        key_prefixes = ["kubernetes.io/"]
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
        bucket  = "datacloud-iam-preprod-tfstate"
        key     = "infra-preprod"
        region  = "eu-central-1"
    }
}

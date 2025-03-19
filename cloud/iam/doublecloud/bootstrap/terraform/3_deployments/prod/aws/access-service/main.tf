locals {
    infra = {
        name = "prod"
    }
    aws_account = "937784353709"
    aws_profile = "doublecloud-aws-prod"

    app              = "access-service"
    provider         = "aws"
    k8s_cluster_name = "iam-${local.infra.name}"
}

provider "aws" {
    profile = local.aws_profile
    region  = "eu-central-1"
    allowed_account_ids = [local.aws_account]
}

terraform {
    backend "s3" {
        profile = "doublecloud-aws-prod"
        bucket  = "datacloud-iam-prod-tfstate"
        key     = "app-access-service"
        region  = "eu-central-1"
    }
}

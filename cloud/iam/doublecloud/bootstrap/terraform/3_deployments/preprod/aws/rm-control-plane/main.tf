locals {
    infra = {
        name = "preprod"
    }
    aws_account = "821159050485"
    aws_profile = "doublecloud-aws-preprod"

    app              = "rm-control-plane"
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
        profile = "doublecloud-aws-preprod"
        bucket  = "datacloud-iam-preprod-tfstate"
        key     = "app-rm-control-plane"
        region  = "eu-central-1"
    }
}

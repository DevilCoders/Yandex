module "aws" {
    source = "../../../modules/infra/clouds/aws/v1"
}

locals {
    default_tags = {
        Infra                                = var.infra_name
        Team                                 = "IAM"
        (module.aws.tags.discounts.all.name) = module.aws.tags.discounts.all.value
    }
}

provider "aws" {
    profile = var.aws_profile
    region  = module.aws.regions[var.region].id
    allowed_account_ids = [var.aws_account]

    default_tags {
        tags = merge(
            local.default_tags,
            {
                Region = module.aws.regions[var.region].id
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
    profile = var.aws_profile
    region  = module.aws.regions[var.region].id
    allowed_account_ids = [var.aws_account]
    alias   = "frankfurt"

    default_tags {
        tags = merge(
            local.default_tags,
            {
                Region = module.aws.regions[var.region].id
            }
        )
    }

    // This is necessary so that tags required for eks can be applied to the vpc without changes to the vpc wiping them out.
    // https://registry.terraform.io/providers/hashicorp/aws/latest/docs/guides/resource-tagging
    ignore_tags {
        key_prefixes = ["kubernetes.io/"]
    }
}

provider "kubernetes" {
    alias = "frankfurt"

    host                   = module.iam_k8s.endpoint
    cluster_ca_certificate = base64decode(module.iam_k8s.certificate_authority_data)
    exec {
        api_version = "client.authentication.k8s.io/v1alpha1"
        args        = ["eks", "get-token", "--cluster-name", module.iam_k8s.name, "--profile", var.aws_profile]
        command     = "aws"
    }
}

provider "kubectl" {
    alias = "frankfurt"

    host                   = module.iam_k8s.endpoint
    cluster_ca_certificate = base64decode(module.iam_k8s.certificate_authority_data)
    exec {
        api_version = "client.authentication.k8s.io/v1alpha1"
        args        = ["eks", "get-token", "--cluster-name", module.iam_k8s.name, "--profile", var.aws_profile]
        command     = "aws"
    }
}

provider "helm" {
    alias = "frankfurt"

    kubernetes {
        host                   = module.iam_k8s.endpoint
        cluster_ca_certificate = base64decode(module.iam_k8s.certificate_authority_data)
        exec {
            api_version = "client.authentication.k8s.io/v1alpha1"
            args        = ["eks", "get-token", "--cluster-name", module.iam_k8s.name, "--profile", var.aws_profile]
            command     = "aws"
        }
    }
    debug = true
}

terraform {
    required_providers {
        aws = {
            source = "hashicorp/aws"
        }
        kubernetes = {
            source  = "hashicorp/kubernetes"
            version = "2.3.2"
        }
        helm = {
            source  = "hashicorp/helm"
            version = "2.2.0"
        }
        kubectl = {
            source = "gavinbunney/kubectl"
            version = "1.13.0"
        }
    }
}

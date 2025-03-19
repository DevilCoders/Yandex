terraform {
  required_version = "0.14.9"
  required_providers {
    grafana = {
      source  = "grafana/grafana"
      version = "1.18.0"
    }
    sops    = {
      source  = "carlpett/sops"
      version = "0.6.3"
    }
  }
  backend "s3" {
    profile = "doublecloud-aws-prod"
    bucket  = "datacloud-infraplane-tfstate"
    key     = "infraplane-iam-prod"
    region  = "eu-central-1"
  }
}

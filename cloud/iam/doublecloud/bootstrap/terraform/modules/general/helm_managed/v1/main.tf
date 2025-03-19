provider "kubernetes" {
  host                   = var.kubernetes_provider.cluster_endpoint
  cluster_ca_certificate = base64decode(var.kubernetes_provider.cluster_certificate_authority_data)
  exec {
    api_version = var.kubernetes_provider.api_version
    args        = var.kubernetes_provider.kubernetes_provider_args
    command     = var.kubernetes_provider.kubernetes_provider_command
  }
}

provider "helm" {
  kubernetes {
    host                   = var.kubernetes_provider.cluster_endpoint
    cluster_ca_certificate = base64decode(var.kubernetes_provider.cluster_certificate_authority_data)
    exec {
      api_version = var.kubernetes_provider.api_version
      args        = var.kubernetes_provider.kubernetes_provider_args
      command     = var.kubernetes_provider.kubernetes_provider_command
    }
  }
  debug = true
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
    helm = {
      source  = "hashicorp/helm"
      version = "2.2.0"
    }
    sops = {
      source  = "carlpett/sops"
      version = "0.6.3"
    }
  }
}

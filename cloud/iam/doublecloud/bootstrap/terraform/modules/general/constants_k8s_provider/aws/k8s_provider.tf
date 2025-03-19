variable "aws_profile" {
  type = string
}

variable "k8s_cluster_name" {
  type = string
}

data "aws_eks_cluster" "cluster" {
  name = var.k8s_cluster_name
}

output "kubernetes_provider" {
  value = {
    cluster_endpoint                   = data.aws_eks_cluster.cluster.endpoint
    cluster_certificate_authority_data = data.aws_eks_cluster.cluster.certificate_authority[0].data
    api_version                        = "client.authentication.k8s.io/v1alpha1"
    kubernetes_provider_command        = "aws"
    kubernetes_provider_args           = ["eks", "get-token", "--cluster-name", data.aws_eks_cluster.cluster.name, "--profile", var.aws_profile]
  }
}

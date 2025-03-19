// Registering cluster's OpenID connect provider in IAM
// This is needed to authenticate/authorize k8s cluster service accounts with IAM in AWS APIs
// See `sa_with_iam_role` submodule
//
// More technical details:
// https://marcincuber.medium.com/amazon-eks-with-oidc-provider-iam-roles-for-kubernetes-services-accounts-59015d15cb0c

locals {
  cluster_oidc_cert_url = aws_eks_cluster.cluster.identity[0].oidc[0].issuer
}

data "tls_certificate" "cluster_oidc_certificate" {
  url = local.cluster_oidc_cert_url
}

resource "aws_iam_openid_connect_provider" "cluster" {
  client_id_list  = ["sts.amazonaws.com"]
  thumbprint_list = [data.tls_certificate.cluster_oidc_certificate.certificates[0].sha1_fingerprint]
  url             = local.cluster_oidc_cert_url
}

locals {
  kubeconfig = templatefile("${path.module}/templates/kubeconfig.tpl", {
    kubeconfig_name                   = "eks_${var.cluster_name}"
    endpoint                          = coalescelist(aws_eks_cluster.cluster[*].endpoint, [""])[0]
    cluster_auth_base64               = coalescelist(aws_eks_cluster.cluster[*].certificate_authority[0].data, [""])[0]
    aws_authenticator_command         = "aws"
    aws_authenticator_command_args    = ["--region", data.aws_region.current.name, "eks", "get-token", "--cluster-name", var.cluster_name, "--profile", var.aws_profile]
    aws_authenticator_additional_args = []
    aws_authenticator_env_variables   = {}
  })
}

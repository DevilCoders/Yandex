data "aws_eks_cluster" "cluster" {
  name = var.k8s_cluster_name
}

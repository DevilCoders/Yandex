output "name" {
  value = var.cluster_name
}

output "cluster_id" {
  value = aws_eks_cluster.cluster.id
}

output "endpoint" {
  value = aws_eks_cluster.cluster.endpoint
}

output "certificate_authority_data" {
  value = aws_eks_cluster.cluster.certificate_authority[0].data
}

output "cluster_role_id" {
  value = aws_iam_role.cluster.id
}

output "cluster_security_group_ids" {
  value = aws_eks_cluster.cluster.vpc_config[*].cluster_security_group_id
}

output "openid_cluster_url" {
  value = aws_iam_openid_connect_provider.cluster.url
}

output "openid_cluster_arn" {
  value = aws_iam_openid_connect_provider.cluster.arn
}

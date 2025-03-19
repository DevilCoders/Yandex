module "metrics" {
  source = "../metrics/v1"

  count = var.skip_metrics_resources ? 0 : 1

  providers = {
    aws     = aws.frankfurt
    helm    = helm.frankfurt
    kubectl = kubectl.frankfurt
  }

  vpc_id                                 = aws_vpc.vpc_iam_frankfurt.id
  amp_subnet_ids                         = local.private_subnets_ids
  amp_security_group_ids                 = module.iam_k8s.cluster_security_group_ids

  k8s_cluster_name                       = module.iam_k8s.name

  openid_cluster_url                     = module.iam_k8s.openid_cluster_url
  openid_cluster_arn                     = module.iam_k8s.openid_cluster_arn

  prometheus_reader_accounts             = var.prometheus_reader_accounts
}

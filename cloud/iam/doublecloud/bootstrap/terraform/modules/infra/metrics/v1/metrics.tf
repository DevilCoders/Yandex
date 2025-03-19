module "amp" {
  source = "../../amp/v1"

  vpc_id             = var.vpc_id
  subnet_ids         = var.amp_subnet_ids
  security_group_ids = var.amp_security_group_ids
}

module "prometheus_sa" {
  source = "../../k8s_sa/v1"

  sa_name            = "prometheus"
  cluster_name       = var.k8s_cluster_name
  namespace          = "prometheus"
  openid_cluster_url = var.openid_cluster_url
  openid_cluster_arn = var.openid_cluster_arn
  policy             = <<POLICY
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "aps:RemoteWrite",
                "aps:QueryMetrics",
                "aps:GetSeries",
                "aps:GetLabels",
                "aps:GetMetricMetadata"
            ],
            "Resource": "*"
        }
    ]
}
POLICY
}

module "prometheus" {
  source = "../../../general/helm/v1"

  name          = "prometheus-for-amp"
  repository    = "https://prometheus-community.github.io/helm-charts"
  chart         = "prometheus"
  chart_version = "14.2.1"
  namespace     = module.prometheus_sa.namespace

  values = [
    templatefile("${path.module}/amp_ingest_override_values.yaml", {
      cluster_name        = var.k8s_cluster_name
    })
  ]

  set_values = [
    {
      name  = "serviceAccounts.server.name"
      value = module.prometheus_sa.sa_name
    },
    {
      name  = "serviceAccounts.server.annotations.eks\\.amazonaws\\.com/role-arn"
      value = module.prometheus_sa.role_arn
    },
    {
      name  = "server.remoteWrite[0].url"
      value = "https://aps-workspaces.${module.amp.region_id}.amazonaws.com/workspaces/${module.amp.workspace_id}/api/v1/remote_write"
    },
    {
      name  = "server.remoteWrite[0].sigv4.region"
      value = module.amp.region_id
    },
  ]
}

module "prometheus_reader" {
  source = "../../../general/prometheus_reader/v1"

  count = length(var.prometheus_reader_accounts) > 0 ? 1 : 0

  prometheus_reader_accounts = var.prometheus_reader_accounts
}

module "constants_k8s_provider" {
  source           = "../../../../modules/general/constants_k8s_provider/aws"
  aws_profile      = local.aws_profile
  k8s_cluster_name = local.k8s_cluster_name
}

module "datacloud_common" {
  source = "../../../../modules/general/helm_managed/v1"

  name             = local.app
  chart            = "../../../../../../helm/datacloud-common"
  env_name         = local.infra.name
  cloud_provider   = local.provider

  kubernetes_provider = module.constants_k8s_provider.kubernetes_provider
}

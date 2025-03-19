module "constants_k8s_provider" {
  source = "../../../../modules/general/constants_k8s_provider/yc"
}

module "iam_sync" {
  source = "../../../../modules/general/helm_managed/v1"

  name             = local.app
  chart            = "../../../../../../helm/datacloud-iam-sync"
  env_name         = local.infra.name
  cloud_provider   = local.provider

  kubernetes_provider = module.constants_k8s_provider.kubernetes_provider
}

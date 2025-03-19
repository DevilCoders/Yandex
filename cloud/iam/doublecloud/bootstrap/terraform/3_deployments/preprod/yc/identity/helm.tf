module "constants_k8s_provider" {
  source = "../../../../modules/general/constants_k8s_provider/yc"
}

module "identity" {
  source = "../../../../modules/general/helm_managed/v1"

  name             = local.app
  chart            = "../../../../../../helm/datacloud-deployment"
  env_name         = local.infra.name
  cloud_provider   = local.provider

  kubernetes_provider = module.constants_k8s_provider.kubernetes_provider
}

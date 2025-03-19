provider "ycp" {
    prod = true
}

module "ai_service_instance_group" {
  source          = "../../../common/services_proxy_ycp"
  yandex_token    = var.yandex_token
  name            = "ai-services-proxy-translate"
  environment     = "prod"
  yc_folder       = "b1g0jv3qc7326nfuig84"
  yc_sa_id        = "aje9mr68nah17loie6pt"

  app_image = "services-proxy-translate"
  solomon_shard_service = "translate"

  yc_instance_group_size = 6
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting
}

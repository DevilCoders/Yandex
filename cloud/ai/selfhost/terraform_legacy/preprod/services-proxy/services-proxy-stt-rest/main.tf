provider "ycp" {
    prod = true
}

module "ai_service_instance_group" {
  source          = "../../../common/services_proxy_ycp"
  yandex_token    = var.yandex_token
  name            = "ai-services-proxy-stt-rest"
  environment     = "preprod"
  yc_folder       = "b1gd3ibutes0q72uq8uf"
  yc_sa_id        = "ajejsom8kdna3r695d9q"

  app_image = "services-proxy-v1rest"

  yc_instance_group_size = 2
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting
}

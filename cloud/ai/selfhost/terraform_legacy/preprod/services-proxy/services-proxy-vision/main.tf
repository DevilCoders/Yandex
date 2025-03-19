provider "ycp" {
    prod = true
}

module "ai_service_instance_group" {
  source          = "../../../common/services_proxy_vision"
  yandex_token    = var.yandex_token
  name            = "ai-services-proxy-vision"
  environment     = "preprod"
  yc_folder       = "b1gd3ibutes0q72uq8uf"
  yc_sa_id        = "ajejsom8kdna3r695d9q"

  app_image = "services-proxy-vision"

  yc_instance_group_size = 1
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting
}

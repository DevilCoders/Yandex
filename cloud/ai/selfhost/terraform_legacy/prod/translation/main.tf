module "ai_translation_service_instance_group" {
  source          = "../../common/translation"
  yandex_token    = var.yandex_token
  name            = "ai-translation-service"
  environment     = "prod"
  yc_folder       = "b1g2cv477a5m35t90v7v"
  yc_sa_id        = "aje11jcredpcejqtj3mr"

  yc_instance_group_size = 0
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  translation_server_version   = "latest"
  services_proxy_version       = "0.95"
}

module "ai_stt_service_instance_group" {
  source          = "../base"
  yandex_token    = var.yandex_token
  name            = "ai-stt-service-aurelius-hotfix1-private"

  yc_instance_group_size = var.yc_instance_group_size
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  stt_server_version = "ru-cloud-general-6680369"
}
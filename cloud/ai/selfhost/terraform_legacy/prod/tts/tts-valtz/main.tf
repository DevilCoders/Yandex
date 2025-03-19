module "ai_tts_service_instance_group" {
  source       = "../base"
  yandex_token = var.yandex_token
  name         = "ai-tts-service-valtz-new"

  yc_instance_group_size = var.yc_instance_group_size
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  instance_cores  = "8"
  instance_memory = "96"
  instance_max_memory = "80"
  instance_gpus = "1"

  tts_server_version           = "ru_valtz_e2e-7365523"
}

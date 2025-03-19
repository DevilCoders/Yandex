module "ai_tts_service_instance_group" {
  source       = "../base"
  yandex_token = var.yandex_token
  name         = "ai-tts-service-oksana-rc"

  yc_instance_group_size = var.yc_instance_group_size
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  instance_cores = "16"
  instance_memory= "192"
  instance_max_memory = "184"
  instance_gpus = "2"
  yc_zones = ["ru-central1-a"]

  tts_server_version = "ru_multispeaker_e2e-7863773"
}

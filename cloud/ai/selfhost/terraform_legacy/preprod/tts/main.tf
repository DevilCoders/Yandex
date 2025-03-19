module "ai_tts_service_instance_group" {
  source          = "../../common/tts_np"
  yandex_token    = var.yandex_token
  name            = "ai-tts-service-ycp"
  environment     = "preprod"
  yc_folder       = "b1gt5rndig3dasvkpid4"
  yc_sa_id        = "ajee8lk9qtr90k9k3eq1"

  yc_instance_group_size = 1
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  tts_server_version = "ru-cloud-general-9220917"

 # image_id = "fd8lkv7hrp2tm05hd42t"
  instance_gpus   = 1
  instance_cores  = 28
  instance_memory = 119
  instance_max_memory = 80
  platform_id="gpu-private-v3"
}

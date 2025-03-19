module "ai_stt_service_instance_group" {
  source       = "../base-ycp"
  yandex_token = var.yandex_token
  name         = "ai-stt-general-deprecated-v3"

  yc_instance_group_size = var.yc_instance_group_size

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  stt_server_version = "9152499_v3"
  stt_model_image_id = "fd8pboqfg70orm7otjj0"

  instance_gpus   = 1
  instance_cores  = 8
  instance_memory = 48
  instance_max_memory = 40
}

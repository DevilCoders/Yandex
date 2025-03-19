module "ai_stt_service_instance_group" {
  source       = "../base-ycp"
  yandex_token = var.yandex_token
  name         = "ai-stt-general-v3"

  yc_instance_group_size = var.yc_instance_group_size

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  stt_server_version = "9637830_v3"
  stt_model_image_id = "fd82u7cn9btdb7607bm8"

  instance_gpus   = 1
  instance_cores  = 28
  instance_memory = 119
  instance_max_memory = 80
  platform_id="gpu-private-v3"
}

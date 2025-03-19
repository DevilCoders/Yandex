module "ai_stt_service_instance_group" {
  source       = "../base-ycp"
  yandex_token = var.yandex_token
  name         = "ai-stt-service-galen"

  yc_instance_group_size = var.yc_instance_group_size

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting
  yc_zones        = var.yc_zones

  instance_gpus   = 1
  instance_cores  = 8
  instance_memory = 96
  instance_max_memory = 88
  platform_id = "gpu-standard-v1"

  stt_server_version = "7651081"
  stt_model_image_id = "fd87frsk7ngpqpan4eeh"
}

module "ai_stt_service_instance_group" {
  source       = "../base-ycp"
  yandex_token = var.yandex_token
  name         = "ai-stt-service-biovitrum"

  yc_instance_group_size = var.yc_instance_group_size

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  stt_server_version = "8769489"
  stt_model_image_id = "fd8op1i6n0760bppn17a"

  instance_gpus   = 1
  instance_cores  = 8
  instance_memory = 48
  instance_max_memory = 40
}

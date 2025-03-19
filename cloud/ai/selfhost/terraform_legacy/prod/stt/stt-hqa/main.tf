module "ai_stt_service_instance_group" {
  source       = "../base-ycp"
  yandex_token = var.yandex_token
  name         = "ai-stt-service-hqa"

  yc_instance_group_size = var.yc_instance_group_size

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  stt_server_version = "7860432"
  stt_model_image_id = "fd8taph6mbhl5nbfl1t6"
}

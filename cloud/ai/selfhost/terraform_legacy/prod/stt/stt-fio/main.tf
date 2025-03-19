module "ai_stt_service_instance_group" {
  source       = "../base-ycp-cpu"
  yandex_token = var.yandex_token
  name         = "ai-stt-service-fio"

  yc_instance_group_size = var.yc_instance_group_size

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  stt_server_version = "7269756"
  stt_model_image_id = "fd8qb8kb1gtgpo6fp6so"
}

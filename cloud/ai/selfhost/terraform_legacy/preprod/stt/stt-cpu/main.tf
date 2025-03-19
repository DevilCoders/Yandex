module "ai_stt_service_instance_group" {
  source       = "../../../common/stt_ycp_cpu"
  yandex_token = var.yandex_token
  name         = "ai-stt-service-cpu"
  environment  = "preprod"
  yc_folder    = "b1gndji7iaubpghf15b0"
  yc_sa_id     = "ajeb8m7oncq1moa3tjr7"

  yc_instance_group_size = var.yc_instance_group_size

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  stt_server_version = "7358209"
  stt_model_image_id = "fd8qb8kb1gtgpo6fp6so"
}


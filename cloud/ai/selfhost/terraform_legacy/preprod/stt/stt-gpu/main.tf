module "ai_stt_service_instance_group" {
  source          = "../../../common/stt_ycp"
  yandex_token    = var.yandex_token
  name            = "ai-stt-service"
  environment     = "preprod"
  yc_folder       = "b1gndji7iaubpghf15b0"
  yc_sa_id        = "ajeb8m7oncq1moa3tjr7"
  # id            = "cl136k54g0n5smkfee13"

  yc_instance_group_size = 2
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  instance_gpus   = 1
  instance_cores  = 28
  instance_memory = 119
  instance_max_memory = 80
  platform_id="gpu-private-v3"

  stt_server_version = "9216069-v3"
  stt_model_image_id = "fd896jjnpkgd6u1hnfii"
}

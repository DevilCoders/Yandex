module "ai_tts_service_instance_group" {
  source       = "../../../common/tts_adaptive"
  name         = "ai-tts-adaptive-poc"
  yandex_token = var.yandex_token
  environment  = "prod"
  yc_folder    = "b1gb294dat6q3ehreoet"
  yc_sa_id     = "ajek01t38m2k5gcbua4c"

  yc_instance_group_size = var.yc_instance_group_size
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  image_id = "fd8sefjts5o3cfjaos0q"

  tts_poc_version = "0.16"
}

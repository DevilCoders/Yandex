module "ai_tts_service_instance_group" {
  source = "../../../common/tts_np"
  yandex_token = var.yandex_token
  name         = var.name
  environment  = "prod"
  yc_folder    = "b1gb294dat6q3ehreoet"
  yc_sa_id     = "ajek01t38m2k5gcbua4c"

  yc_instance_group_size = var.yc_instance_group_size

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting
  yc_zones        = var.yc_zones

  instance_cores  = var.instance_cores
  instance_memory = var.instance_memory
  instance_max_memory = var.instance_max_memory
  instance_gpus = var.instance_gpus
  image_id = var.image_id

  tts_server_version           = var.tts_server_version
}

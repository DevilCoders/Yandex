module "ai_stt_service_instance_group" {
  source       = "../../../common/stt_ycp"
  yandex_token = var.yandex_token
  name         = var.name
  environment  = "prod"
  yc_folder    = "b1gos77c2en2ek4br48e"
  yc_sa_id     = "ajetcebmqrguvq01bfmn"

  yc_instance_group_size = var.yc_instance_group_size

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  stt_server_version = var.stt_server_version
  stt_model_image_id = var.stt_model_image_id
 # image_id = "fd8lkv7hrp2tm05hd42t"

  yc_zones = var.yc_zones
  platform_id = var.platform_id
  instance_gpus = var.instance_gpus
  instance_cores = var.instance_cores
  instance_memory = var.instance_memory
  instance_max_memory = var.instance_max_memory
}


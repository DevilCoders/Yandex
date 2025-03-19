module "ai_stt_service_instance_group" {
  source       = "../base-ycp"
  yandex_token = var.yandex_token
  name         = "ai-stt-service-anaximander-hotfix1"

  yc_instance_group_size = var.yc_instance_group_size

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  stt_server_version = "7269756"
  stt_model_image_id = "fd8tj1shncejinsu1rkj"

  yc_zones = ["ru-central1-b"]
  platform_id = "gpu-standard-v1"
  instance_gpus = "1"
  instance_cores = "8"
}

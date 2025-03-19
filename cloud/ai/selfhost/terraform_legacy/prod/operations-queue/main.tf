module "ai_service_instance_group" {
  source          = "../../common/operations_queue"
  yandex_token    = var.yandex_token
  name            = var.name
  environment     = "prod"
  yc_folder       = "b1guo0vio0pjndaqcs3q"
  yc_sa_id        = "ajemunv4at1fegtqh921"

  yc_instance_group_size = var.yc_instance_group_size
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting
}

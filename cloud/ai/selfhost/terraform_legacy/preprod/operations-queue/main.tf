module "ai_service_instance_group" {
  source          = "../../common/operations_queue"
  yandex_token    = var.yandex_token
  name            = "ai-operations-queue"
  environment     = "preprod"
  yc_folder       = "b1ge0dhndro3sir4aj8l"
  yc_sa_id        = "ajefkp6b0dgcouq02vdr"

  yc_instance_group_size = 1
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting
}

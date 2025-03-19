module "speechkit_operations_queue_staging" {
  source = "../common"

  // Secrets
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  // Target
  environment        = "staging"
  folder_id          = "b1ge0dhndro3sir4aj8l"
  service_account_id = "ajefkp6b0dgcouq02vdr"

  instance_group_size = 1
}

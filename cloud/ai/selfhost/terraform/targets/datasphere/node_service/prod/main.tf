module "node_service_prod" {
  source = "../common"

  // Secrets
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  // Target
  environment        = "prod"
  folder_id          = "b1g4ujiu1dq5hh9544r6"
  service_account_id = "ajepu21fhq5taau4qb4h"

  instance_group_size = 3
}


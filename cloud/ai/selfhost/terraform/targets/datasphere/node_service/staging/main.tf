module "node_service_staging" {
  source = "../common"

  // Secrets
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  // Target
  environment        = "staging"
  folder_id          = "b1g19hobememv3hj6qsc"
  service_account_id = "ajec1jspgsvif0dgrao1"
}


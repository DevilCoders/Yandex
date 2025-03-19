module "node_deployer_staging" {
  source = "../common"

  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  environment        = "staging"
  folder_id          = "b1g19hobememv3hj6qsc"
  service_account_id = "ajec1jspgsvif0dgrao1"

  s3_access_key = var.s3_access_key
  s3_secret_key = var.s3_secret_key
}

module "node_deployer_preprod" {
  source = "../common"

  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  environment        = "preprod"
  folder_id          = "aoef893gt5c5ivo37deq"
  service_account_id = "bfbdf2s3k8gc5tqciqrg"

  s3_access_key = var.s3_access_key
  s3_secret_key = var.s3_secret_key
}

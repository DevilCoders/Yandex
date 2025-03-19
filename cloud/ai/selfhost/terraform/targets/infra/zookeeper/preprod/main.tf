module "zookeeper_preprod" {
  source = "../common"

  // Secrets
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  // Target
  environment        = "preprod"
  folder_id          = "aoe490a9dtdqsm3g7bhm" # infra
  service_account_id = "bfbl2jcv3ek2qaqfig3s" # infra-deployer-testing
}

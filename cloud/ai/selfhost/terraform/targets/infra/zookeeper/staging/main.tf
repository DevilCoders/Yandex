module "zookeeper_staging" {
  source = "../common"

  // Secrets
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  // Target
  environment        = "staging"
  folder_id          = "b1gdgmbfkh6f4rlf5puu" # app-zk-preprod TODO -> infra-staging
  service_account_id = "ajejueub6j5hpecpo6ig" # zk-deployer-preprod TODO -> infra-deployer-staging
}

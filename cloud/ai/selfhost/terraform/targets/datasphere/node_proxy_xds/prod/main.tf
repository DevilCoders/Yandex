module "node_proxy_xds_prod" {
  source = "../common"

  // Secrets
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file
  tvm_secret               = var.tvm_secret

  // Target
  environment        = "prod"
  folder_id          = "b1g4ujiu1dq5hh9544r6"
  service_account_id = "ajepu21fhq5taau4qb4h"
}

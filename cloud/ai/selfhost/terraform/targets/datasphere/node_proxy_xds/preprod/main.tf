module "node_proxy_xds_service_preprod" {
  source = "../common"

  // Secrets
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file
  tvm_secret               = var.tvm_secret

  // Target
  environment        = "preprod"
  folder_id          = "aoef893gt5c5ivo37deq"
  service_account_id = "bfbdf2s3k8gc5tqciqrg"
}
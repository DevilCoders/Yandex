module "node_proxy_staging" {
  source = "../common"

  // Secrets
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  // Target
  environment        = "staging"
  folder_id          = "b1g19hobememv3hj6qsc"
  service_account_id = "ajec1jspgsvif0dgrao1"

  // Required args
  // TODO: Hide behind the ????
  // envoy_node_proxy_xds_address = "node-proxy-xds-staging-1.datasphere.cloud.yandex.net"

  tvm_secret = var.tvm_secret
}

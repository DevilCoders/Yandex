module "node_proxy_preprod" {
  source = "../common"

  // Secrets
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  // Target
  environment        = "preprod"
  folder_id          = "aoef893gt5c5ivo37deq"
  service_account_id = "bfbdf2s3k8gc5tqciqrg"

  // Required args
  // TODO: Hide behind the ????
  # xds_servers = {
  #   ru-central1-a = "node-proxy-xds-preprod-1.datasphere.cloud-preprod.yandex.net"
  #   ru-central1-b = "node-proxy-xds-preprod-1.datasphere.cloud-preprod.yandex.net"
  #   ru-central1-c = "node-proxy-xds-preprod-1.datasphere.cloud-preprod.yandex.net"
  # }

  tvm_secret = var.tvm_secret
}
module "gore-preprod" {
  source = "../modules/gore-api"

  environment          = "preprod"
  service_account_id   = module.accounts_and_keys.gore_sa_id
  subnet_ids           = module.network.subnet_ids
  dns_zone_id          = "dns2t4fobecr3kg2evoq"
  docker_image_version = "0.1.69-debian"
  network_id           = module.network.network_id
  tvm_secret           = var.tvm_secret
  mongo_users          = var.mongo_users
  api_domain           = "resps-api-preprod.oncall.cloud.yandex.net"
  need_dns_record      = true
}

module "gore-prod" {
  source = "../modules/gore-api"

  environment          = "prod"
  service_account_id   = module.accounts_and_keys.gore_sa_id
  subnet_ids           = module.network.subnet_ids
  dns_zone_id          = "dns2t4fobecr3kg2evoq"
  docker_image_version = "0.1.69-debian"
  network_id           = module.network.network_id
  tvm_secret           = var.tvm_secret
  mongo_users          = var.mongo_users
  api_domain           = "resps-api.cloud.yandex.net"
  need_dns_record      = false
}

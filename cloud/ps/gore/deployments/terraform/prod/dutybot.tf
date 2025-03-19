module "dutybot-preprod" {
  source = "../modules/dutybot"

  environment = "preprod"
  service_account_id = module.accounts_and_keys.gore_sa_id
  network_id = module.network.network_id
  subnet_ids = module.network.subnet_ids
  dns_zone_id = "dns2t4fobecr3kg2evoq"
  docker_image_version = "1.4.16"
  bot_id = "409568234"
  hc_network_ipv6 = "2a0d:d6c0:2:ba::/80"

  run_yandex_messenger_bot = false
}

module "dutybot-prod" {
  source = "../modules/dutybot"

  environment = "prod"
  service_account_id = module.accounts_and_keys.gore_sa_id
  network_id = module.network.network_id
  subnet_ids = module.network.subnet_ids
  dns_zone_id = "dns2t4fobecr3kg2evoq"
  docker_image_version = "1.4.16"
  bot_id = "551825859"
  hc_network_ipv6 = "2a0d:d6c0:2:ba::/80"
}

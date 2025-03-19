module "yav_secret_stt_deployer_access_key" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = module.constants.by_environment.common_secret
  value_name = "stt-deployer-access-key"
}

module "yav_secret_stt_deployer_secret_key" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = module.constants.by_environment.common_secret
  value_name = "stt-deployer-secret-key"
}

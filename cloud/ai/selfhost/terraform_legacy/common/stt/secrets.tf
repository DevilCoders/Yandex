// TVM secret for logbroker.yandex.net
module "yav_secret_yc_ai_tvm" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = module.constants.by_environment.env_secret
  value_name = "tvm-secret"
}

// TVM secret for logbroker.yandex.net (billing)
module "yav_secret_yc_ai_tvm_billing" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = module.constants.by_environment.env_secret
  value_name = "tvm-secret-billing"
}

// Private key for "data" folder service account
module "yav_secret_yc_ai_data_sa_private_key" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = module.constants.by_environment.env_secret
  value_name = "data-sa-private-key"
}

module "yav_secret_yc_ai_redis-password" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = module.constants.by_environment.env_secret
  value_name = "managed-redis-password"
}
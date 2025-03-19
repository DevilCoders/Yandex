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
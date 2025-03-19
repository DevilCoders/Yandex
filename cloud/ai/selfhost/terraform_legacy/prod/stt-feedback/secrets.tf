locals {
  yav_secret_cloud_ai_id = "sec-01dzc5wk8dsvmfqff4rf04jx0d"
}

module "yav_secret_yc_ai_api-key-yandex" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = local.yav_secret_cloud_ai_id
  value_name = "tests-sa-api-key"
}

module "yav_secret_yc_ai_api-key-tinkoff" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = local.yav_secret_cloud_ai_id
  value_name = "tinkoff-jwt-token"
}

module "yav_secret_yc_ai_api-key-google" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = local.yav_secret_cloud_ai_id
  value_name = "google-api-key"
}
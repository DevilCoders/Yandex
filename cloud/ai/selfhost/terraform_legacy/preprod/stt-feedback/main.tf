provider "ycp" {
    prod = true
}

module "ai_service_instance_group" {
  source          = "../../common/services_proxy_ycp"
  yandex_token    = var.yandex_token
  name            = "ai-stt-feedback"
  environment     = "preprod"
  yc_folder       = "b1gooapscoase9vh1jip"
  yc_sa_id        = "ajejd23og2dr9c6gpa8u"

  app_image = "stt-feedback"
  app_version = "0.223"

  yc_instance_group_size = 1
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  kms_key_id = "abj6udaickg0idclsk9o"
  encrypted_dek = "AAAAAQAAABRhYmptOWpsbjRyaXVqcTUydjN0ZwAAABDyMrfoTl6L2bO7w/l2T91qAAAADPiOYNeEKWXO7RZ2NNmBsCU1l/wbXFaf0hEGOcLjFFLcrKHW2N722mRMwxGY+CCvnsXpxmQ6aY9vJfszww5tXR3Iixv0kGxam6qODvEX6Ld37qtgRdiahjBDalqs"

  custom_env_vars = [
    {
      name = "API_KEY_yandex"
      value: module.yav_secret_yc_ai_api-key-yandex.secret
    },
    {
      name = "API_KEY_tinkoff"
      value: module.yav_secret_yc_ai_api-key-tinkoff.secret
    },
    {
      name = "API_KEY_google"
      value: module.yav_secret_yc_ai_api-key-google.secret
    },
  ]
}

provider "ycp" {
    prod = true
}

module "ai_service_instance_group" {
  source          = "../../common/services_proxy_ycp"
  yandex_token    = var.yandex_token
  name            = "ai-stt-feedback"
  environment     = "prod"
  yc_folder       = "b1g2tpck4um2iufss8gm"
  yc_sa_id        = "aje9lsrd2c3k09ip4er3"

  app_image = "stt-feedback"
  app_version = "0.223"

  yc_instance_group_size = 3
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  kms_key_id = "abjn4iuq1ei8c2s7eue9"
  encrypted_dek = "AAAAAQAAABRhYmp1aTZsbGlyNXZyYjE4ZXZxbgAAABCVpuuTRmFg8THE427AZUJEAAAADOoQpEDAFnTzyvIW8NIbY8L9TM7OmD8hVqPAZ+DMqBFTJwn7QtfUK7JCKv1Gk2LhlG1hiSyFIL3qgejH9mfL/gr8Bnyaf+YH++hl5aX2pyw6Sw8Cgu2APk9agqtB"

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

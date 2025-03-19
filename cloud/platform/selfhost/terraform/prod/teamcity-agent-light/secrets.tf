locals {
  docker_auth = coalesce(
    var.docker_auth,
    module.yav-secret-docker_auth.value,
    "there-is-no-valid-value-detected!",
  )
  docker_auth_cr_yandex = coalesce(
    var.docker_auth_cr_yandex,
    module.yav-secret-docker_auth_cr_yandex.value,
    "there-is-no-valid-value-detected!",
  )

  docker_auth_cr_yandex_preprod = coalesce(
  var.docker_auth_cr_yandex,
  module.yav-secret-docker_auth_cr_yandex_preprod.value,
  "there-is-no-valid-value-detected-for-preprod!",
  )
}

module "yav-secret-docker_auth" {
  source    = "../../modules/yav"
  secret_id = "sec-01dqymp8adchjcf21cyfhgcnkv"
  key_id    = "auth_string"
}

module "yav-secret-docker_auth_cr_yandex" {
  source    = "../../modules/yav"
  secret_id = "sec-01ewg8678mrs284jn1j3v2r6s4"
  key_id    = "docker_config_auth"
}

module "yav-secret-docker_auth_cr_yandex_preprod" {
  source    = "../../modules/yav"
  secret_id = "sec-01ewmsdrv2r3v8xvb82wm306x2"
  key_id    = "docker_config_auth"
}


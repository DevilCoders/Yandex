// TODO: get rid of it all, move to skm-based approach.
locals {
  docker_auth = coalesce(
    var.docker_auth,
    module.yav-secret-docker_auth.value,
    "there-is-no-valid-value-detected!",
  )

  juggler_token = coalesce(
    var.juggler_token,
    module.yav-secret-juggler_token.value,
    "there-is-no-valid-value-detected!",
  )

  solomon_token = coalesce(
    var.solomon_token,
    module.yav-secret-solomon_token.value,
    "there-is-no-valid-value-detected!",
  )
}

module "yav-secret-docker_auth" {
  source    = "../../modules/yav"
  secret_id = "sec-01cx8a81rj3458rhqaj5x1sztb"
  key_id    = "auth_string"
}

module "yav-secret-juggler_token" {
  source    = "../../modules/yav"
  secret_id = "sec-01dqfyqhpf6vbsbtkfk02nswkc"
  key_id    = "JUGGLER_OAUTH_TOKEN"
}

module "yav-secret-solomon_token" {
  source    = "../../modules/yav"
  secret_id = "sec-01dwf7ypx34h8z2sf89kqhbqyj"
  key_id    = "OAuth"
}


locals {
  docker_auth = coalesce(
    var.docker_auth,
    module.yav-secret-docker_auth.value,
    "there-is-no-valid-value-detected!",
  )

  aws_access_key = coalesce(
    var.aws_access_key,
    module.yav-secret-aws_access_key.value,
    "there-is-no-valid-value-detected!",
  )
  aws_secret_key = coalesce(
    var.aws_secret_key,
    module.yav-secret-aws_secret_key.value,
    "there-is-no-valid-value-detected!",
  )
}

module "yav-secret-docker_auth" {
  source    = "../../modules/yav"
  secret_id = "sec-01cx8a81rj3458rhqaj5x1sztb"
  key_id    = "auth_string"
}

module "yav-secret-aws_access_key" {
  source    = "../../modules/yav"
  secret_id = "sec-01dq4s00brvkx8nwre9ab3vqg5"
  key_id    = "access-key-id"
}

module "yav-secret-aws_secret_key" {
  source    = "../../modules/yav"
  secret_id = "sec-01dq4s00brvkx8nwre9ab3vqg5"
  key_id    = "secret"
}

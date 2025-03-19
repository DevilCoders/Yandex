locals {
  docker_auth = coalesce(
    var.docker_auth,
    module.yav-secret-docker_auth.value,
    "there-is-no-valid-value-detected!",
  )

  envoy_cert = var.cert_file != "_intentionally_empty_file" ? file(var.cert_file) : module.yav-secret-envoy_cert.value
  envoy_key  = var.key_file != "_intentionally_empty_file" ? file(var.key_file) : module.yav-secret-envoy_key.value
}

module "yav-secret-docker_auth" {
  source    = "../../modules/yav"
  secret_id = "sec-01cx8a81rj3458rhqaj5x1sztb"
  key_id    = "auth_string"
}

module "yav-secret-envoy_cert" {
  source    = "../../modules/yav"
  secret_id = "sec-01d3rn7xzknnb99r01w1e6km81"
  key_id    = "crt"
}

module "yav-secret-envoy_key" {
  source    = "../../modules/yav"
  secret_id = "sec-01d3rn7xzknnb99r01w1e6km81"
  key_id    = "key"
}


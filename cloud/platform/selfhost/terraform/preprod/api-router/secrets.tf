locals {
  docker_auth = coalesce(
    var.docker_auth,
    module.yav-secret-docker_auth.value,
    "there-is-no-valid-value-detected!",
  )

  xds_client_cert = var.xds_client_cert_file != "_intentionally_empty_file" ? file(var.xds_client_cert_file) : module.yav-secret-xds_client_cert.value
  xds_client_key  = var.xds_client_key_file != "_intentionally_empty_file" ? file(var.xds_client_key_file) : module.yav-secret-xds_client_key.value

  envoy_cert = var.cert_file != "_intentionally_empty_file" ? file(var.cert_file) : module.yav-secret-envoy_cert.value
  envoy_key  = var.key_file != "_intentionally_empty_file" ? file(var.key_file) : module.yav-secret-envoy_key.value
}

module "yav-secret-envoy_cert" {
  source    = "../../modules/yav"
  secret_id = "sec-01d45d0kgxhppgpjy54jk2hwmm"
  key_id    = "crt"
}

module "yav-secret-envoy_key" {
  source    = "../../modules/yav"
  secret_id = "sec-01d45d0kgxhppgpjy54jk2hwmm"
  key_id    = "key"
}

module "yav-secret-xds_client_cert" {
  source    = "../../modules/yav"
  secret_id = "sec-01d16g9cr9kyqn8vvkcjsaa0ts"
  key_id    = "crt"
}

module "yav-secret-xds_client_key" {
  source    = "../../modules/yav"
  secret_id = "sec-01d16g9cr9kyqn8vvkcjsaa0ts"
  key_id    = "key"
}

module "yav-secret-docker_auth" {
  source    = "../../modules/yav"
  secret_id = "sec-01cx8a81rj3458rhqaj5x1sztb"
  key_id    = "auth_string"
}

module "yav-secret-terraform-state-preprod" {
  source    = "../../modules/yav"
  secret_id = "sec-01cwa619wea9r9ya23g61jdm8q"
  key_id    = "secret_key"
}


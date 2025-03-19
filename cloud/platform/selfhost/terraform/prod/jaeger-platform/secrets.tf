// Secrets can be fetched via yav module.

module "yc_token" {
  source = "../../modules/yc-token"
}

module "docker_auth" {
  source    = "../../modules/yav"
  secret_id = "sec-01cx8a81rj3458rhqaj5x1sztb"
  key_id    = "auth_string"
}

module "ssh-keys" {
  source             = "../../modules/ssh-keys"
  yandex_token       = module.oauth.result
  abc_service        = "yc-tracing"
  abc_service_scopes = ["devops"]
}

module "oauth" {
  source = "../../modules/yav-oauth"
}

module "jaeger-lb-token" {
  source    = "../../modules/yav"
  secret_id = "sec-01eaxyzdasew1zj43bfyn76ayj"
  key_id    = "lb_token"
}

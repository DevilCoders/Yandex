module "constants" {
  source      = "../../../../constants"
  environment = var.environment
}

module "skm_bundle" {
  source = "../../../../modules/skm_secrets"

  kms_key_id    = module.constants.by_environment.kms.node_deployer.key_id
  encrypted_dek = module.constants.by_environment.kms.node_deployer.encrypted_dek

  environment = var.environment

  yav_token = var.yandex_token

  yav_secrets = {
    node_api_host_cert = {
      secret_id = module.constants.by_environment.datasphera_secret
      path      = "/etc/certs/servercert.pem"
    }
    node_api_host_key = {
      secret_id = module.constants.by_environment.datasphera_secret
      path      = "/etc/certs/serverkey.pem"
    }
  }
}

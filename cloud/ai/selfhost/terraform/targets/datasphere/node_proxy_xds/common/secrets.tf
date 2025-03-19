module "constants" {
  source      = "../../../../constants"
  environment = var.environment
}

module "skm_bundle" {
  source      = "../../../../modules/skm_secrets"
  environment = var.environment

  encrypted_dek = module.constants.by_environment.node_service_encrypted_dek
  kms_key_id    = module.constants.by_environment.node_service_kms_key_id
  yav_token     = var.yandex_token
  yav_secrets   = {}
  file_secrets  = []
}

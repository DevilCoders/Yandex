module "constants" {
  source      = "../../../../constants"
  environment = var.environment
}

module "skm_bundle" {
  source = "../../../../modules/skm_secrets"

  encrypted_dek = module.constants.by_environment.infra_encrypted_dek
  kms_key_id    = module.constants.by_environment.infra_kms_key_id
  environment   = var.environment
  yav_token     = var.yandex_token
  yav_secrets = {
    // FIXME: There is no secrets required by ZK now
    //        Get rid of skm_bundle here
  }
}

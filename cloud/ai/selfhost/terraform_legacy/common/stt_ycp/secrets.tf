module "skm_bundle" {
    source = "../../modules/skm_secrets"
    encrypted_dek = module.constants.by_environment.stt_encrypted_dek
    kms_key_id = module.constants.by_environment.stt_kms_key_id
    yav_token = var.yandex_token
    yav_secrets = {
        tvm-secret = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/secrets/tvm_secret"
        }
        tvm-secret-billing = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/secrets/tvm_secret_billing"
        }
        data-sa-private-key = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/yc/ai/keys/data-sa/private.pem"
        }
        push-client-producer-key = {
          secret_id = module.constants.by_environment.common_secret
          path = "/etc/yandex/statbox-push-client/key.json"
        }
    }
}

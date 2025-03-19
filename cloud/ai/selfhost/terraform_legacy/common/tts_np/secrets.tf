module "skm_bundle" {
    source = "../../modules/skm_secrets"
    encrypted_dek = module.constants.by_environment.tts_encrypted_dek
    kms_key_id = module.constants.by_environment.tts_kms_key_id
    yav_token = var.yandex_token
    yav_secrets = {
        tvm-secret = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/secrets/tvm_secret"
        }
        data-sa-private-key = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/yc/ai/keys/data-sa/private.pem"
        }
        logbroker-publisher-private-key = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/yc/ai/keys/data-sa/logbroker-publisher-private.pem"
        }
        push-client-producer-key = {
          secret_id = module.constants.by_environment.common_secret
          path = "/etc/yandex/statbox-push-client/key.json"
        }
    }
}

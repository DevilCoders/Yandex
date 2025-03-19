module "skm_bundle" {
    source = "../../modules/skm_secrets"
    encrypted_dek = var.encrypted_dek != "" ? var.encrypted_dek : module.constants.by_environment.zk_encrypted_dek
    kms_key_id = var.kms_key_id != "" ? var.kms_key_id : module.constants.by_environment.zk_kms_key_id
    yav_token = var.yandex_token
    yav_secrets = {
        tvm-secret-billing = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/secrets/tvm_secret_billing"
        }
        data-sa-private-key = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/yc/ai/keys/data-sa/private.pem"
        }
        managed-redis-password = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/secrets/managed_redis_password"
        }
        surrogate-api-key = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/secrets/surrogate_api_key"
        }
        push-client-producer-key = {
            secret_id = module.constants.by_environment.common_secret
            path = "/etc/yandex/statbox-push-client/key.json"
        }
    }
}

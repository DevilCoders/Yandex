module "skm_bundle" {
    source = "../../modules/skm_secrets"
    encrypted_dek = module.constants.by_environment.encrypted_dek
    kms_key_id = module.constants.by_environment.kms_key_id
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
        data-sa-public-key = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/yc/ai/keys/data-sa/public.pem"
        }
        managed-redis-password = {
            secret_id = module.constants.by_environment.env_secret
            path = "/etc/secrets/managed_redis_password"
        }
        vision-textdetection-api-key = {
            secret_id = module.constants.by_environment.common_secret
            path = "/etc/secrets/vision_textdetection_api_key"
        }
        push-client-producer-key = {
            secret_id = module.constants.by_environment.common_secret
            path = "/etc/yandex/statbox-push-client/key.json"
        }
    }
    file_secrets = [
        {
            path: "/etc/yc/ai/keys/deployer-sa/private.pem"
            content: module.solomon_config.sa_private_key
        },
        {
            path: "/etc/yc/ai/keys/deployer-sa/public.pem"
            content: module.solomon_config.sa_public_key
        }
    ]
}


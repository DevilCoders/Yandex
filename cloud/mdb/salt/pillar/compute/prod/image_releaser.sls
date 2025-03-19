data:
    image-releaser:
        mode: "compute"
        stable_duration: "4h"
        sentry:
            dsn: {{ salt.yav.get('ver-01e49s0rwn57m5g6h2mrzv8p77[dsn]') }}
            environment: "compute-prod"
        compute:
            s3:
                bucket: "a2bd1a42-69fd-451a-8dee-47ba04fbacbf"
                client:
                    host: "storage.yandexcloud.net"
                    access_key: {{ salt.yav.get('ver-01e64b245m6a0b02hhjmt5a21w[id]') }}
                    secret_key: {{ salt.yav.get('ver-01e64b245m6a0b02hhjmt5a21w[secret]') }}
            s3_url_prefix: "https://storage.yandexcloud.net/a2bd1a42-69fd-451a-8dee-47ba04fbacbf"
            destination:
                folder: "b1gdepbkva865gm1nbkq"
                compute_url: "compute-api.cloud.yandex.net:9051"
                token_service_url: "ts.private-api.cloud.yandex.net:4282"
                service_account:
                    id: {{ salt.yav.get('ver-01e6gcj03677w20g4bb4xjptyh[account_id]') }}
                    key_id: {{ salt.yav.get('ver-01e6gcj03677w20g4bb4xjptyh[key_id]') }}
                    private_key: |
                      {{ salt.yav.get('ver-01e6gcj03677w20g4bb4xjptyh[private_key]') | indent(22) }}

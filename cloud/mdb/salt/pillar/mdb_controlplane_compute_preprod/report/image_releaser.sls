data:
    image-releaser:
        no-check: True
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
                folder: "aoed5i52uquf5jio0oec"
                compute_url: "compute-api.cloud-preprod.yandex.net:9051"
                keep_images: 2
                service_account:
                    id: {{ salt.yav.get('ver-01e6er72x6taw4gmj4dzfd01xg[account_id]') }}
                    key_id: {{ salt.yav.get('ver-01e6er72x6taw4gmj4dzfd01xg[key_id]') }}
                    private_key: {{ salt.yav.get('ver-01e6er72x6taw4gmj4dzfd01xg[private_key]') | yaml_encode }}

include:
    - mdb_controlplane_compute_preprod.report.images

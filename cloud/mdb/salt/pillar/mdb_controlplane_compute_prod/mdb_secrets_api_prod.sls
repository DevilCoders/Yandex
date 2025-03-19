mine_functions:
    grains.item:
        - id
        - ya
        - virtual

cert.key: |
    {{ salt.yav.get('ver-01fegsy48r1cbw25p6jy0y3a35[private]') | indent(4) }}
cert.crt: |
    {{ salt.yav.get('ver-01fegsy48r1cbw25p6jy0y3a35[cert]') | indent(4) }}

data:
    runlist:
        - components.network
        - components.nginx
        - components.jaeger-agent
        - components.mdb-secrets
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.dbaas-compute-controlplane
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
        use_yasmagent: False
    solomon:
        cluster: secrets-api
    common:
        dh: |
            {{ salt.yav.get('ver-01e8aa6yky10erg6egz2fc8dn9[token]') | indent(12) }}
    mdb-secrets:
        server_name: mdb-secrets.private-api.cloud.yandex.net
        config:
            app:
                sentry:
                    dsn: {{ salt.yav.get('ver-01f9kv20wppcvnddsre9ckxvbj[dsn]') }}
            iam_enabled: True
            iam:
                host: as.private-api.cloud.yandex.net:4286
                cloud_id: b1ggh9onj7ljr7m9cici
            blackbox_enabled: False
            saltkey: {{ salt.yav.get('ver-01fedqfdeschzjqzd2b9qbmgmw[salt_public_key]') }}
            privatekey: {{ salt.yav.get('ver-01fedqfdeschzjqzd2b9qbmgmw[secrets_private_key]') }}
            certificate_to_use: yandex
            yandex_crt:
                oauth: {{ salt.yav.get('ver-01g748k3bqr3ybhsae7eyhxz3d[token]') }}
                abc: 1895
                url: https://crt-api.yandex-team.ru/api/certificate/
            secretsdb:
                addrs:
                 - secrets-db-prod01f.yandexcloud.net:6432
                 - secrets-db-prod01h.yandexcloud.net:6432
                 - secrets-db-prod01k.yandexcloud.net:6432
                user: secrets_api
                password: {{ salt.yav.get('ver-01e8a9pd6d5naecfj315kqyk59[password]') }}
                max_open_conn: 32
                max_idle_conn: 32

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.deploy
    - mdb_controlplane_compute_prod.common.solomon
    - compute.prod.jaeger_agent

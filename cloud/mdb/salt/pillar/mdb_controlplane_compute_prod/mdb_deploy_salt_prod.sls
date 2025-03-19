data:
    runlist:
        - components.deploy.salt-master
        - components.redis
        - components.dbaas-compute-controlplane
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.mdb-controlplane-telegraf
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    monrun:
        http_url: 'https://localhost'
    monrun2: True
    system:
        journald:
            system_max_use: '10G'
    ipv6selfdns: True
    cauth_use: False
    second_selfdns: True
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        cluster: mdb_deploy_salt_compute_prod
    use_yasmagent: True
    dbaas:
        vtype: compute
    salt_master:
        worker_threads: 256
        sock_pool_size: 32
        log_level: warn
        use_prometheus_metrics: true
    telegraf:
        prometheus:
            interval: '1s'
    s3:
        host: storage.yandexcloud.net
        access_key_id: {{ salt.yav.get('ver-01eza54h5397v6nb5hrf9k6bpc[access_key]') }}
        access_secret_key: {{ salt.yav.get('ver-01eza54h5397v6nb5hrf9k6bpc[secret_key]') }}
    walg:
        enabled: False
    iam_ts: 'ts.private-api.cloud.yandex.net:4282'
    salt_version: 3002.7+ds-1+yandex0
    salt_py_version: 3

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.solomon
    - compute.prod.salt.vault_key
    - compute.prod.salt.redis_config
    - compute.prod.salt.keys_v2
    - compute.prod.salt.ext_pillars
    - compute.prod.mdb_deploy_salt

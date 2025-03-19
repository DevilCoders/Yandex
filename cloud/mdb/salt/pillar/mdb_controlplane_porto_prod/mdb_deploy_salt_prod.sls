data:
    runlist:
        - components.deploy.salt-master
        - components.redis
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.mdb-salt-sync
        - components.dbaas-porto-controlplane
        - components.mdb-controlplane-telegraf
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    monrun:
        http_url: 'https://localhost'
    monrun2: True
    system:
        journald:
            system_max_use: '50G'
    use_yasmagent: True
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_deploy_salt_porto_prod
        service: salt
    salt_master:
        worker_threads: 48
        sock_pool_size: 30
        log_level: info
        use_prometheus_metrics: true
    walg:
        enabled: False
    telegraf:
        prometheus:
            interval: '1s'
    salt_version: 3002.7+ds-1+yandex0
    salt_py_version: 3

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - porto.prod.salt.vault_key
    - porto.prod.salt.redis_config
    - porto.prod.salt.keys_v2
    - porto.prod.salt.ext_pillars
    - porto.prod.dbaas.mdb_deploy_salt
    - porto.prod.dbaas.solomon
    - porto.prod.s3.pgaas_s3backup

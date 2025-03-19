data:
    deploy:
        version: 2
        api_host: deploy-api-test.db.yandex-team.ru
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
        service: salt
    salt_master:
        worker_threads: 16
        log_level: debug
        s3_images_bucket: 'mdb-salt-images-test'
    walg:
        enabled: False
    salt_version: 3002.7+ds-1+yandex0
    salt_py_version: 3

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.selfdns
    - porto.test.salt.vault_key
    - porto.test.salt.redis_config
    - porto.test.salt.keys_v2
    - porto.test.ext_pillars
    - porto.test.dbaas.mdb_deploy_salt
    - porto.test.dbaas.solomon
    - porto.test.s3.pgaas_s3backup

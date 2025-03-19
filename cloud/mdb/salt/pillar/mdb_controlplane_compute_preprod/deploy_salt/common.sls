data:
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    monrun:
        http_url: 'https://localhost'
    monrun2: True
    system:
        journald:
            system_max_use: '15G'
    ipv6selfdns: True
    cauth_use: False
    second_selfdns: True
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    use_yasmagent: True
    dbaas:
        vtype: compute
    salt_master:
        worker_threads: 16
        log_level: debug
    s3:
        host: storage.cloud-preprod.yandex.net
        access_key_id: {{ salt.yav.get('ver-01eywp6fbax5ajhy0bxxc6ht0v[access_key]') }}
        access_secret_key: {{ salt.yav.get('ver-01eywp6fbax5ajhy0bxxc6ht0v[secret_key]') }}
    walg:
        enabled: False
    common:
        dh: |
            {{ salt.yav.get('ver-01dwh7qyxh4hqn15g3eh59y2zw[private]') | indent(12) }}
    mdb-deploy-saltkeys:
        config:
            deploy_api_token: {{ salt.yav.get('ver-01dwh7wm7jxcyyjrn7w7sj0r82[token]') }}
            saltapi:
                user: mdb-deploy-salt-api
                password: {{ salt.yav.get('ver-01dwh7y9nev29ywmspenwx39mf[password]') }}
                eauth: pam
    mdb-deploy-salt-api:
        salt_api_user: mdb-deploy-salt-api
        salt_api_password: {{ salt.yav.get('ver-01dwh81dytxe0cm5fjdsk94866[password]') }}
    iam_ts: ts.private-api.cloud-preprod.yandex.net:4282
    salt_version: 3002.7+ds-1+yandex0
    salt_py_version: 3

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - compute.preprod.salt.vault_key
    - compute.preprod.salt.redis_config
    - compute.preprod.salt.keys_v2
    - compute.preprod.salt.ext_pillars
    - compute.preprod.robot-pgaas-deploy_ssh_key

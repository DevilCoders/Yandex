data:
    runlist:
        - components.deploy.salt-master
        - components.dbaas-compute-controlplane
    dbaas:
        vtype: porto
    monrun2: True
    ipv6selfdns: True
    cauth_use: False
    second_selfdns:
        domain: -prod.db.yandex.net
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/salt_getter.py
    salt_version: '3000.2+ds-1+yandex0'
    salt_master:
        use_deploy: False
    s3:
        host: storage.yandexcloud.net
        access_key_id: {{ salt.yav.get('ver-01edefgtgxxs0a2e3avm6ntxhm[access_key]') }}
        access_secret_key: {{ salt.yav.get('ver-01edefgtgxxs0a2e3avm6ntxhm[secret_key]') }}

include:
    - envs.qa
    - mdb_controlplane_compute_prod.common
    - compute.prod.salt.redis_config
    - compute.prod.salt.keys_v2
    - compute.prod.salt.ext_pillars
    - compute.prod.mdb_deploy_salt
    - compute.prod.solomon

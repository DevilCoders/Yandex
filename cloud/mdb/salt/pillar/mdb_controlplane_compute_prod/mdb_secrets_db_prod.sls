{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    auto_resetup: True
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    runlist:
        - components.postgres
        - components.pg-dbs.secretsdb
        - components.dbaas-compute-controlplane
    certs:
        readonly: True
    pgsync:
        zk_hosts: zk-dbaas01f.yandexcloud.net:2181,zk-dbaas01h.yandexcloud.net:2181,zk-dbaas01k.yandexcloud.net:2181
    ipv6selfdns: True
    monrun2: True
    sysctl:
        vm.nr_hugepages: 0
    dbaas:
        vtype: compute
    config:
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        shared_buffers: 1GB
        maintenance_work_mem: 512MB
        archive_timeout: 10min
        max_connections: 400
    cauth_use: False
    connection_pooler: odyssey
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        project: yandexcloud
        cluster: 'mdb_{ctype}'
        service: yandexcloud_dbaas
    use_yasmagent: True
    s3_bucket: yandexcloud-dbaas-secrets

{% set env = 'compute-prod' %}
{% set pg_major_version = 12 %}
{% set edition = 'default' %}
    versions:
        postgres:
            major_version: {{ pg_major_version }}
            minor_version: {{ pg_versions_package_map[(env, pg_major_version * 100, edition)]['minor_version'] | tojson }}
            package_version: {{ pg_versions_package_map[(env, pg_major_version * 100, edition)]['package_version'] }}
            edition: {{ edition }}
        odyssey:
            major_version: {{ pg_major_version }}
            minor_version: {{ odyssey_versions_package_map[(env, pg_major_version * 100, edition)]['minor_version'] | tojson }}
            package_version: {{ odyssey_versions_package_map[(env, pg_major_version * 100, edition)]['package_version'] }}
            edition: {{ edition }}

include:
    - envs.{{ env }}
    - mdb_controlplane_compute_prod.common
    - compute.prod.pgusers.common
    - compute.prod.pgusers.secrets_api
    - compute.prod.s3.mdb_s3backup
    - compute.prod.secrets.secrets-db-prod01-gpg

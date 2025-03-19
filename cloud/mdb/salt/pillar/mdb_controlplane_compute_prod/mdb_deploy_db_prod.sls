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
    runlist:
        - components.postgres
        - components.pg-dbs.deploydb
        - components.dbaas-compute-controlplane
    deploydb:
        target: 58
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    sysctl:
        vm.nr_hugepages: 0
    config:
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        shared_buffers: 8GB
        maintenance_work_mem: 1GB
        max_connections: 800
        archive_timeout: 10min
        lock_timeout: 60s
    pgsync:
        zk_hosts: zk-dbaas01f.yandexcloud.net:2181,zk-dbaas01h.yandexcloud.net:2181,zk-dbaas01k.yandexcloud.net:2181
        failover_checks: 60
    ipv6selfdns: True
    monrun2: True
    use_yasmagent: True
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: mdb_deploy_db_compute_prod
        service: yandexcloud_dbaas
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
    dbaas:
        vtype: compute
    cauth_use: False
    connection_pooler: odyssey
    s3_bucket: yandexcloud-dbaas-deploydb

include:
    - envs.{{ env }}
    - mdb_controlplane_compute_prod.common
    - compute.prod.pgusers.common
    - compute.prod.pgusers.deploy_api
    - compute.prod.pgusers.deploy_cleaner
    - compute.prod.pgusers.mdb_ui
    - compute.prod.pgusers.deploydb_admin
    - compute.prod.s3.mdb_s3backup
    - compute.prod.solomon
    - mdb_deploydb_common

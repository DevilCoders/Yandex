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
        - components.pg-dbs.katandb
        - components.dbaas-compute-controlplane
    katandb:
        target: latest
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    pgsync:
        zk_hosts: zk-dbaas01f.yandexcloud.net:2181,zk-dbaas01h.yandexcloud.net:2181,zk-dbaas01k.yandexcloud.net:2181
        zk_lockpath_prefix: /pgsync/katandb
    gpg:
        cluster_name: mdb_katandb01
    s3:
        gpg_key_id: mdb-katandb01
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
    ipv6selfdns: True
    sysctl:
        vm.nr_hugepages: 0
    config:
        shared_buffers: 1GB
        maintenance_work_mem: 512MB
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        min_wal_size: 1GB
        max_wal_size: 1GB
    dbaas:
        vtype: compute
    cauth_use: False
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: 'mdb_katandb01'
        service: yandexcloud_dbaas
    use_yasmagent: True
    s3_bucket: yandexcloud-dbaas-katandb-cp

include:
    - envs.{{ env }}
    - mdb_controlplane_compute_prod.common
    - compute.prod.s3.mdb_s3backup
    - compute.prod.pgusers.common
    - compute.prod.pgusers.katan
    - compute.prod.pgusers.katandb_admin
    - compute.prod.pgusers.katan_imp
    - compute.prod.solomon
    - compute.prod.pgusers.mdb_ui

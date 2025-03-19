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
        - components.pg-dbs.dbaas_metadb
        - components.dbaas-compute-controlplane
    config:
        lock_timeout: 10s
        shared_buffers: 8GB
        maintenance_work_mem: 515MB
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        min_wal_size: 1GB
        wal_level: logical
    sysctl:
        vm.nr_hugepages: 0
    pgsync:
        zk_hosts: zk-dbaas01f.yandexcloud.net:2181,zk-dbaas01h.yandexcloud.net:2181,zk-dbaas01k.yandexcloud.net:2181
    ipv6selfdns: True
    monrun2: True
    dbaas_metadb:
        target: 397
    dbaas:
        vtype: compute
    cauth_use: False
    connection_pooler: odyssey
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: 'mdb_{ctype}'
        service: yandexcloud_dbaas
    use_yasmagent: True
    s3_bucket: yandexcloud-dbaas-metadb

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
    - compute.prod.pgusers.dbaas_api
    - compute.prod.pgusers.dbaas_worker
    - compute.prod.pgusers.mdb_health
    - compute.prod.pgusers.mdb_report
    - compute.prod.pgusers.mdb_dns
    - compute.prod.pgusers.idm_service
    - compute.prod.pgusers.dbaas_support
    - compute.prod.pgusers.mdb_search_producer
    - compute.prod.pgusers.mdb_event_producer
    - compute.prod.pgusers.katan_imp
    - compute.prod.pgusers.mdb_maintenance
    - compute.prod.pgusers.mdb_ui
    - compute.prod.pgusers.dataproc_health
    - compute.prod.pgusers.backup_scheduler
    - compute.prod.pgusers.backup_worker
    - compute.prod.pgusers.backup_cli
    - compute.prod.pgusers.cms
    - compute.prod.pgusers.cloud_dwh
    - compute.prod.pgusers.logs_api
    - compute.prod.pgusers.mdb_downtimer
    - compute.prod.pgusers.metadb_admin
    - compute.prod.pgusers.pillar_config
    - compute.prod.pgusers.pillar_secrets
    - compute.prod.pgusers.billing_bookkeeper
    - compute.prod.pgusers.mdb_reaper
    - compute.prod.pgusers.mdb_exporter
    - compute.prod.metadb
    - compute.prod.s3.mdb_s3backup
    - compute.prod.solomon

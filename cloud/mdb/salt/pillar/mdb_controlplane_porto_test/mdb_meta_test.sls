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
        - components.pg-dbs.dbaas_metadb
        - components.dbaas-porto-controlplane
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    config:
        shared_buffers: 1GB
        max_connections: 1024
        lock_timeout: 60s
        wal_log_hints: 'on'
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        autovacuum_vacuum_cost_delay: 20ms
        autovacuum_vacuum_cost_limit: 1000
        pool_mode: transaction
        wal_level: logical
    pgsync:
        zk_hosts: zk-df-e2e01f.db.yandex.net:2181,zk-df-e2e01h.db.yandex.net:2181,zk-df-e2e01k.db.yandex.net:2181
    walg:
        parallel_workers: 8
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: 'mdb_{ctype}'
        service: mdb
    use_yasmagent: True
    dbaas_metadb:
        target: latest
{% set env = 'dev' %}
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
    - index_repack
    - envs.{{ env }}
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.grpc_internal_api
    - porto.prod.selfdns.realm-mdb
    - porto.prod.pgusers.common
    - porto.test.pgusers.metadb_admin
    - porto.prod.s3.pgaas_s3backup
    - porto.prod.pgusers.dev.dbaas_api
    - porto.prod.pgusers.dbaas_worker
    - porto.prod.pgusers.mdb_report
    - porto.prod.pgusers.idm_service
    - porto.prod.pgusers.dbaas_support
    - porto.prod.pgusers.mdb_health
    - porto.prod.pgusers.mdb_dns
    - porto.test.pgusers.mdb_maintenance
    - porto.test.metadb
    - porto.prod.pgusers.dev.mdb_search_producer
    - porto.prod.pgusers.dev.mdb_event_producer
    - porto.test.pgusers.deploydb_admin
    - porto.test.pgusers.katan_imp
    - porto.test.pgusers.mdb_ui
    - porto.test.pgusers.backup_scheduler
    - porto.test.pgusers.backup_worker
    - porto.test.pgusers.backup_cli
    - porto.test.pgusers.cloud_dwh
    - porto.test.pgusers.cms
    - porto.test.pgusers.dataproc_health
    - porto.test.pgusers.logs_api
    - porto.test.pgusers.mdb_downtimer
    - porto.test.pgusers.pillar_config
    - porto.test.pgusers.pillar_secrets
    - porto.test.pgusers.billing_bookkeeper
    - porto.test.pgusers.mdb_reaper
    - porto.test.pgusers.mdb_exporter
    - porto.prod.dbaas.solomon

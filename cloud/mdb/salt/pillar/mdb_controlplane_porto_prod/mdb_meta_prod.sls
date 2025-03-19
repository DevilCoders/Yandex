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
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        wal_level: logical
    pgsync:
        zk_hosts: zkeeper01e.db.yandex.net:2181,zkeeper01f.db.yandex.net:2181,zkeeper01h.db.yandex.net:2181
    walg:
        parallel_workers: 8
    monrun2: True
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: 'mdb_{ctype}'
        service: mdb
    use_yasmagent: True
    dbaas_metadb:
        target: 397

{% set env = 'prod' %}
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
    - mdb_controlplane_porto_prod.common
    - mdb_controlplane_porto_prod.common.grpc_internal_api
    - porto.prod.pgusers.common
    - porto.prod.s3.pgaas_s3backup
    - porto.prod.pgusers.dbaas_api
    - porto.prod.pgusers.dbaas_worker
    - porto.prod.pgusers.mdb_report
    - porto.prod.pgusers.idm_service
    - porto.prod.pgusers.dbaas_support
    - porto.prod.pgusers.mdb_health
    - porto.prod.pgusers.mdb_dns
    - porto.prod.pgusers.mdb_maintenance
    - porto.prod.metadb
    - porto.prod.pgusers.mdb_search_producer
    - porto.prod.pgusers.mdb_event_producer
    - porto.prod.pgusers.katan_imp
    - porto.prod.pgusers.mdb_ui
    - porto.prod.pgusers.backup_scheduler
    - porto.prod.pgusers.backup_worker
    - porto.prod.pgusers.backup_cli
    - porto.prod.pgusers.cloud_dwh
    - porto.prod.pgusers.cms
    - porto.prod.pgusers.dataproc_health
    - porto.prod.pgusers.logs_api
    - porto.prod.pgusers.mdb_downtimer
    - porto.prod.pgusers.metadb_admin
    - porto.prod.pgusers.pillar_config
    - porto.prod.pgusers.pillar_secrets
    - porto.prod.pgusers.billing_bookkeeper
    - porto.prod.pgusers.mdb_reaper
    - porto.prod.pgusers.mdb_exporter
    - porto.prod.dbaas.solomon

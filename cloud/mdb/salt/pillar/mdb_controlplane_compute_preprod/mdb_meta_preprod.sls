{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    migrates_in_k8s: True
    auto_resetup: True
    runlist:
        - components.postgres
        - components.pg-dbs.dbaas_metadb
        - components.dbaas-compute-controlplane
        - components.dbaas-cron
        - components.pg-dbs.unmanaged.perf-diag-enabled
        - components.pushclient2
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    config:
        shared_buffers: 2GB
        maintenance_work_mem: 515MB
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        min_wal_size: 1GB
        max_wal_size: 1GB
        log_min_duration_statement: 100ms
        lock_timeout: 10s
        default_statistics_target: 1000
        wal_level: logical
    pgsync:
        zk_lockpath_prefix: meta-dbaas-preprod
        zk_hosts: zk-dbaas-preprod01f.cloud-preprod.yandex.net:2181,zk-dbaas-preprod01h.cloud-preprod.yandex.net:2181,zk-dbaas-preprod01k.cloud-preprod.yandex.net:2181
    sysctl:
        vm.nr_hugepages: 0
    ipv6selfdns: True
    monrun2: True
    dbaas_metadb:
        target: latest
    dbaas:
        vtype: compute
    cauth_use: False
    solomon:
        cluster: 'mdb_{ctype}'
    use_yasmagent: True
    s3_bucket: yandexcloud-dbaas-metadb
    perf_diag:
        tvm_secret: {{ salt.yav.get('ver-01emnpshzbhx2k7y0nwrzp6hfz[secret]') }}
        topics:
{# use porto-prod perfdiag because there is grafana for it #}
            pg_stat_activity: mdb/porto/prod/perf_diag/pg_stat_activity
            pg_stat_statements: mdb/porto/prod/perf_diag/pg_stat_statements
            pg_stat_statements_query: mdb/porto/prod/perf_diag/pg_stat_statements_query
        tvm_client_id: 2016999
        tvm_server_id: 2001059
        pgsa_sample_period: 1
        pgss_sample_period: 60
        enable: True
        cluster_id: mdb-meta-preprod
        cluster_name: mdb-meta-preprod
{% set env = 'qa' %}
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
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - mdb_controlplane_compute_preprod.common.grpc_internal_api
    - compute.preprod.pgusers.common
    - compute.preprod.pgusers.metadb_admin
    - compute.preprod.pgusers.dbaas_api
    - compute.preprod.pgusers.dbaas_worker
    - compute.preprod.pgusers.mdb_health
    - compute.preprod.pgusers.mdb_report
    - compute.preprod.pgusers.mdb_dns
    - compute.preprod.pgusers.idm_service
    - compute.preprod.pgusers.dbaas_support
    - compute.preprod.pgusers.mdb_search_producer
    - compute.preprod.pgusers.mdb_event_producer
    - compute.preprod.pgusers.katan_imp
    - compute.preprod.pgusers.logs_api
    - compute.preprod.pgusers.mdb_maintenance
    - compute.preprod.pgusers.mdb_ui
    - compute.preprod.pgusers.dataproc_health
    - compute.preprod.pgusers.backup_scheduler
    - compute.preprod.pgusers.backup_worker
    - compute.preprod.pgusers.backup_cli
    - compute.preprod.pgusers.mdb_downtimer
    - compute.preprod.pgusers.deploydb_admin
    - compute.preprod.pgusers.cms
    - compute.preprod.pgusers.cloud_dwh
    - compute.preprod.pgusers.pillar_config
    - compute.preprod.pgusers.pillar_secrets
    - compute.preprod.pgusers.billing_bookkeeper
    - compute.preprod.pgusers.mdb_reaper
    - compute.preprod.pgusers.mdb_exporter
    - compute.preprod.metadb
    - compute.preprod.s3.mdb_s3backup

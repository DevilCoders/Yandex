{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    sysctl:
        vm.nr_hugepages: 5000
    runlist:
        - components.postgres
        - components.pg-dbs.s3db
        - components.linux-kernel
        - components.dbaas-compute.underlay
    connection_pooler: odyssey
    config:
        effective_cache_size: 64GB
        work_mem: 64MB
        synchronous_commit: remote_write
        max_prepared_transactions: 10
        log_min_duration_statement: 50ms
        checkpoint_timeout: 30min
        default_statistics_target: 1000
        autovacuum_vacuum_cost_delay: 5ms
        autovacuum_vacuum_cost_limit: 2000
        maintenance_work_mem: 2GB
        lock_timeout: 200ms
        wal_sender_timeout: 15s
        wal_receiver_timeout: 15s
        max_standby_streaming_delay: 1s
        autovacuum_max_workers: 20
{% set edition = 'default' %}
{% if salt['grains.get']('id').split('.')[0][:-1][4:] in ['05', '06', '07'] %}
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon,pg_pathman
{% set pg_major_version = 11 %}
{% else %}
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
{% set pg_major_version = 12 %}
{% endif %}
        auto_kill_timeout: 60 minutes
{% set env = 'compute-prod' %}
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
    s3pgmeta: 'host=pgmeta01f.svc.cloud.yandex.net,pgmeta01h.svc.cloud.yandex.net,pgmeta01k.svc.cloud.yandex.net port=6432 dbname=s3db user=s3ro'
    s3db:
        target: 151
        update_chunks_counters:
            batch_size: 2000000
        chunk_splitter:
            enable: True
        chunk_merger:
            enable: False
        copy_delete_queue:
            enable: False
        sentry_dsn: {{ salt.yav.get('ver-01f5e2zwfq9e1at9y53wbc5bsf[secret]') }}
    s3_closer:
        pgmeta: 'host=pgmeta01f.svc.cloud.yandex.net,pgmeta01h.svc.cloud.yandex.net,pgmeta01k.svc.cloud.yandex.net port=6432 dbname=s3db'
        db: 'host=localhost dbname=s3db'
    pgbouncer:
        count: 8
    pgsync:
        zk_hosts: zk-s3-compute01f.svc.cloud.yandex.net:2181,zk-s3-compute01h.svc.cloud.yandex.net:2181,zk-s3-compute01k.svc.cloud.yandex.net:2181
    monrun:
        tls:
            warn: 30
        bouncer_log_errors:
            warn: '10\ 10'
            crit: '1000\ 1000'
            exclude: s3cleanup,s3service
        load_average_relative:
            warn: 2
            crit: 4
        pg_log_errors:
            warn: '10\ 100'
            crit: '1000\ 3000'
            ignore_read_only: False
            exclude: '"ERRCODE_LOCK_NOT_AVAILABLE,ERRCODE_UNIQUE_VIOLATION,ERRCODE_S3_InvalidPart,ERRCODE_S3_NoSuchUpload,NOT_KNOWN_ERROR: 67219683"'
        pg_counters_queue:
            dbname: s3db
            schema: s3
            table: chunks_counters_queue
            warn: 1000000
            crit: 3000000
        unispace:
            crit: 95
            warn: 90
        mem_usage:
            check: True
    walg:
        nobalancer: False
        suffix: {{ salt['grains.get']('id').split('.')[0][:-1] }}-compute-prod
        parallel_workers: 8
    mdb_metrics:
        enabled: True
        system_sensors: True
    use_yasmagent: False
    monrun2: True
    cauth_use: False
    ipv6selfdns: True
    kcache:
        excluded_keys: 'request_id'

include:
    - compute_vtype
    - index_repack
    - table_repack
    - envs.{{ env }}
    - compute.prod.s3.solomon
    - compute.prod.selfdns.realm-cloud-mdb
    - compute.prod.s3.pgaas_s3backup
    - compute.prod.s3.pgusers.common
    - compute.prod.s3.pgusers.s3
    - compute.prod.s3.pgusers.pgmeta_s3
    - mdb_s3_compute_prod.kcache

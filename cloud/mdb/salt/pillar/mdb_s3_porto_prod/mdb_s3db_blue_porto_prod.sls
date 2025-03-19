{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

{% set env = 'prod' %}

{% set my_dc = salt['grains.get']('ya:short_dc', 'msk') %}
{% if my_dc == 'man' %}
{% set priority = 0 %}
{% elif my_dc == 'iva' %}
{% set priority = 1 %}
{% elif my_dc == 'sas' or my_dc == 'vla' %}
{% set priority = 5 %}
{% else %}
{% set priority = 10 %}
{% endif %}
mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    connection_pooler: odyssey
    runlist:
        - components.postgres
        - components.pg-dbs.s3db
        - components.dbaas-cron
        - components.pg-dbs.unmanaged.perf-diag-enabled
        - components.pushclient2
        - components.dbaas-billing
    perf_diag:
        tvm_secret: {{ salt.yav.get('ver-01emnpshzbhx2k7y0nwrzp6hfz[secret]') }}
        topics:
            pg_stat_activity: mdb/porto/prod/perf_diag/pg_stat_activity
            pg_stat_statements: mdb/porto/prod/perf_diag/pg_stat_statements
            pg_stat_statements_query: mdb/porto/prod/perf_diag/pg_stat_statements_query
        tvm_client_id: 2016999
        tvm_server_id: 2001059
        pgsa_sample_period: 1
        pgss_sample_period: 60
        enable: True
        cluster_id: {{ salt['grains.get']('pg:suffix','unknown') }}
        cluster_name: {{ salt['grains.get']('pg:suffix','unknown') }}
    pgsync:
        priority: {{ priority }}
        zk_hosts: zkeeper04f.db.yandex.net:2181,zkeeper04h.db.yandex.net:2181,zkeeper04k.db.yandex.net:2181
        remaster_restart: 'no'
    config:
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        max_parallel_workers_per_gather: 0
        max_parallel_workers: 0
        effective_cache_size: 64GB
        wal_sender_timeout: 15s
        wal_receiver_timeout: 15s
        work_mem: 64MB
        synchronous_commit: remote_write
        max_prepared_transactions: 10
        log_min_duration_statement: 50ms
        checkpoint_timeout: 30min
        default_statistics_target: 1000
        log_maxsize: 10G
        autovacuum_vacuum_cost_delay: 5ms
        autovacuum_vacuum_cost_limit: 2000
        autovacuum_max_workers: 20
        lock_timeout: 200ms
        enable_seqscan: 'off'
        max_standby_streaming_delay: 1s
        auto_kill_timeout: 30 minutes
    pgbouncer:
        count: 8
    s3pgmeta: 'host=pgmeta-blue01h.db.yandex.net port=6432 dbname=s3db user=s3ro'
    s3db:
        target: 144
        update_chunks_counters:
            batch_size: 2000000
        purge_objects:
            ttl: '3 days'
            batch_size: 10000
        chunk_splitter:
            enable: True
        chunk_merger:
            exclude_bids: d61cd0c4-4b65-4aad-88ec-6c6aef6e98bf
        copy_delete_queue:
            enable: False
        sentry_dsn: {{ salt.yav.get('ver-01f5e2zwfq9e1at9y53wbc5bsf[secret]') }}
    monrun:
        tls:
            warn: 30
        bouncer_log_errors:
            warn: '50\ 50'
            crit: '2000\ 2000'
            exclude: s3cleanup,s3service
        pg_log_errors:
            warn: '10\ 100'
            crit: '2000\ 3000'
            ignore_read_only: False
            exclude: '"ERRCODE_S3_NoSuchUpload,ERRCODE_LOCK_NOT_AVAILABLE,ERRCODE_UNIQUE_VIOLATION,ERRCODE_T_R_SERIALIZATION_FAILURE,ERRCODE_RAISE_EXCEPTION"'
        pg_counters_queue:
            dbname: s3db
            schema: s3
            table: chunks_counters_queue
            warn: 1000000
            crit: 3000000
        unispace:
            crit: 95
            warn: 90
    s3_closer:
        pgmeta: 'host=pgmeta-blue01h.db.yandex.net port=6432 dbname=s3db'
        db: 'host=localhost dbname=s3db'
    monrun2: True
    do_clear_after_repack: True
    index_repack:
        perc_min: 30
        cpu_limit: 2c
        io_limit: 167772160
    billing:
        tvm_secret: {{ salt.yav.get('ver-01f3qjge269sbrsk7gvz1ht0xh[secret]') }}
        logbroker_host: lbkx.logbroker.yandex.net.
        topic: /yc/yandex/billing-mdb-porto-prod-instance
        tvm_client_id: 2018095
        tvm_server_id: 2001059
        cluster_type: postgresql_cluster
        cloud_id: foo9r60cnim655o9cp5k
        folder_id: fooks8fe0stsver2ll4b
    logship:
        enabled: False
{% set edition = 'default' %}
{% set pg_major_version = 12 %}
    walg:
        storage: ssh
        parallel_workers: 8
        ssh_server: pg-backup03i.db.yandex.net
    auto_resetup: True
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
    - porto.prod.selfdns.realm-mdb
    - porto.prod.walg
    - envs.{{ env }}
    - porto.prod.pgusers.common
    - porto.prod.pgusers.s3_blue
    - porto.prod.pgusers.s3_blue_pgmeta
    - mdb_s3_porto_prod.kcache

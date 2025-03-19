{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
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
    sysctl:
        vm.nr_hugepages: 0
    auto_resetup: True
    runlist:
        - components.postgres
        - components.pg-dbs.s3db
        - components.linux-kernel
        - components.dbaas-compute.read-ahead
        - components.dbaas-compute.underlay
    connection_pooler: odyssey
    dbaas_compute:
        read_ahead_kb: 0
    config:
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        effective_cache_size: 4GB
        shared_buffers: 1GB
        work_mem: 16MB
        maintenance_work_mem: 256MB
        synchronous_commit: remote_write
        max_prepared_transactions: 10
        log_min_duration_statement: 50ms
        checkpoint_timeout: 30min
        default_statistics_target: 1000
        autovacuum_vacuum_cost_delay: 5ms
        autovacuum_vacuum_cost_limit: 2000
        lock_timeout: 200ms
        wal_sender_timeout: 15s
        wal_receiver_timeout: 15s
        max_standby_streaming_delay: 1s
    s3pgmeta: 'host=pgmeta01f.svc.cloud-preprod.yandex.net,pgmeta01h.svc.cloud-preprod.yandex.net,pgmeta01k.svc.cloud-preprod.yandex.net port=6432 dbname=s3db user=s3ro'
    s3db:
        update_chunks_counters:
            batch_size: 2000000
        chunk_splitter:
            enable: True
        sentry_dsn: {{ salt.yav.get('ver-01f5e2zwfq9e1at9y53wbc5bsf[secret]') }}
    s3_closer:
        pgmeta: 'host=pgmeta01f.svc.cloud-preprod.yandex.net,pgmeta01h.svc.cloud-preprod.yandex.net,pgmeta01k.svc.cloud-preprod.yandex.net port=6432 dbname=s3db'
        db: 'host=localhost dbname=s3db'
    pgbouncer:
        count: 2
    pgsync:
        zk_hosts: zk-s3-compute01f.svc.cloud-preprod.yandex.net:2181,zk-s3-compute01h.svc.cloud-preprod.yandex.net:2181,zk-s3-compute01k.svc.cloud-preprod.yandex.net:2181
        remaster_restart: 'no'
    monrun:
        tls:
            warn: 30
        bouncer_log_errors:
            warn: '10\ 10'
            crit: '1000\ 1000'
            exclude: s3cleanup,s3service
        pg_log_errors:
            warn: '10\ 100'
            crit: '1000\ 3000'
            ignore_read_only: False
            exclude: '"ERRCODE_S3_NoSuchUpload,ERRCODE_UNIQUE_VIOLATION,ERRCODE_S3_InvalidPart,NOT_KNOWN_ERROR: 67219683"'
        pg_counters_queue:
            dbname: s3db
            schema: s3
            table: chunks_counters_queue
            warn: 1000000
            crit: 3000000
    walg:
        nobalancer: False
        suffix: {{ salt['grains.get']('id').split('.')[0][:-1] }}-compute-preprod
    mdb_metrics:
        enabled: True
        system_sensors: True
    use_yasmagent: False
    monrun2: True
    cauth_use: False
    ipv6selfdns: True
    do_clear_after_repack: True
    kcache:
        version: '138-02f6178'
        buffer_size: 200
        stat_time_interval: 86400
        excluded_keys: 'request_id'

include:
    - compute_vtype
    - index_repack
    - table_repack
    - envs.{{ env }}
    - compute.preprod.s3.solomon
    - compute.preprod.selfdns.realm-cloud-mdb
    - compute.preprod.s3.pgaas_s3backup
    - compute.preprod.pgusers.common
    - compute.preprod.pgusers.s3
    - compute.preprod.pgusers.pgmeta_s3

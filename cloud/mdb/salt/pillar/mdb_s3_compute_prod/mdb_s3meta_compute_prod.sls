{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
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
    sysctl:
        vm.nr_hugepages: 5000
    runlist:
        - components.postgres
        - components.pg-dbs.s3meta
        - components.linux-kernel
        - components.dbaas-compute.underlay
    connection_pooler: odyssey
    config:
        pgmeta:
            server: pgmeta01f.svc.cloud.yandex.net,pgmeta01h.svc.cloud.yandex.net,pgmeta01k.svc.cloud.yandex.net
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        effective_cache_size: 64GB
        work_mem: 64MB
        max_prepared_transactions: 10
        log_min_duration_statement: 50ms
        maintenance_work_mem: 2GB
        lock_timeout: 200ms
        wal_sender_timeout: 15s
        wal_receiver_timeout: 15s
        default_statistics_target: 1000
    pgsync:
        zk_hosts: zk-s3-compute01f.svc.cloud.yandex.net:2181,zk-s3-compute01h.svc.cloud.yandex.net:2181,zk-s3-compute01k.svc.cloud.yandex.net:2181
    pgbouncer:
        count: 8
    monrun:
        tls:
            warn: 30
        bouncer_log_errors:
            warn: '10\ 10'
            crit: '100\ 100'
        pg_log_errors:
            warn: '10\ 10'
            crit: '100\ 100'
            ignore_read_only: False
            exclude: '"ERRCODE_S3_NoSuchBucket,ERRCODE_S3_BucketAlreadyOwnedByYou,ERRCODE_PROTOCOL_VIOLATION"'
        pg_counters_update:
            dbname: s3meta
            warn: 1000
            crit: 3000
        pg_prepared_xacts:
            all: True
        mem_usage:
            check: True
    s3pgmeta: 'host=pgmeta01f.svc.cloud.yandex.net,pgmeta01h.svc.cloud.yandex.net,pgmeta01k.svc.cloud.yandex.net port=6432 dbname=s3db user=s3ro'
    s3meta:
        target: 107
        chunk_mover:
            enable: True
            auto: True
            max_threads: 4
            allow_same_shard: True
            koeff_chunks_max: 12
        db: 'host=localhost port=6432 dbname=s3meta user=s3util'
        user: 's3util'
        update_chunks_counters:
            threads: 2
        sentry_dsn: {{ salt.yav.get('ver-01f5e2zwfq9e1at9y53wbc5bsf[secret]') }}
    s3_closer:
        pgmeta: 'host=pgmeta01f.svc.cloud.yandex.net,pgmeta01h.svc.cloud.yandex.net,pgmeta01k.svc.cloud.yandex.net port=6432 dbname=s3db'
        db: 'host=localhost dbname=s3meta'
    walg:
        nobalancer: False
        suffix: s3meta01-compute-prod
        parallel_workers: 8
    mdb_metrics:
        enabled: True
        system_sensors: True
    use_yasmagent: False
    monrun2: True
    cauth_use: False
    ipv6selfdns: True
    use_pg_partman: True
    kcache:
        excluded_keys: "'request_id,bucket'"

include:
    - compute_vtype
    - index_repack
    - envs.{{ env }}
    - compute.prod.s3.solomon
    - compute.prod.selfdns.realm-cloud-mdb
    - compute.prod.s3.pgaas_s3backup
    - compute.prod.s3.pgusers.common
    - compute.prod.s3.pgusers.s3
    - compute.prod.s3.pgusers.pgmeta_s3
    - compute.prod.s3.pgusers.pgproxy_common
    - compute.prod.s3.pgusers.pgproxy_pgpass
    - mdb_s3_compute_prod.kcache

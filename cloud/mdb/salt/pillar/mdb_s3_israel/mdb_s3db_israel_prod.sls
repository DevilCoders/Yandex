{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}
{% from "mdb_s3_israel/map.jinja" import clusters, zk_hosts with context %}
{% set pgmeta_fqdns = clusters.pgmeta01.fqdns | join(',') %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    sysctl:
        vm.nr_hugepages: 0
    runlist:
        - components.postgres
        - components.pg-dbs.s3db
        - components.linux-kernel
        - components.deploy.agent
    connection_pooler: odyssey
    config:
        effective_cache_size: {{ (clusters.s3db01.resources.memory * 1024 * 2 / 3 // 1024) | int }}GB
        work_mem: 64MB
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
        autovacuum_max_workers: 20
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        auto_kill_timeout: 60 minutes
{% set edition = 'default' %}
{% set pg_major_version = 12 %}
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
    s3pgmeta: 'host={{ pgmeta_fqdns }} port=6432 dbname=s3db user=s3ro'
    s3db:
        target: 139
        update_chunks_counters:
            batch_size: 2000000
        chunk_splitter:
            enable: True
        chunk_merger:
            enable: False
    s3_closer:
        pgmeta: 'host={{ pgmeta_fqdns }} port=6432 dbname=s3db'
        db: 'host=localhost dbname=s3db'
    pgbouncer:
        count: 8
    pgsync:
        zk_hosts: {{ zk_hosts }}
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
            exclude: '"ERRCODE_LOCK_NOT_AVAILABLE,ERRCODE_UNIQUE_VIOLATION,ERRCODE_S3_InvalidPart,ERRCODE_S3_NoSuchUpload"'
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
        parallel_workers: 8
    mdb_metrics:
        enabled: True
        system_sensors: True
    s3:
       gpg_key: {{ salt.lockbox.get("bcn5r7v28vps4vf2pn4p").gpg_key | tojson }}
       gpg_key_id: {{ salt.lockbox.get("bcn5r7v28vps4vf2pn4p").gpg_key_id | tojson }}
       host: storage-internal.cloud.yandex.net
       access_key_id: {{ salt.lockbox.get("bcn929e77d82gj4k5i8h").access_key_id | tojson }}
       access_secret_key: {{ salt.lockbox.get("bcn929e77d82gj4k5i8h").access_secret_key | tojson }}
       endpoint: https+path://storage-internal.cloud.yandex.net
    s3_bucket: mdb-s3-israel

include:
    - compute_vtype
    - index_repack
    - envs.{{ env }}
    - mdb_s3_israel.common
    - mdb_s3_israel.pgusers.common
    - mdb_s3_israel.pgusers.s3
    - mdb_s3_israel.pgusers.pgmeta_s3
    - mdb_s3_compute_prod.kcache


{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}
{% from "mdb_s3_israel/map.jinja" import clusters, zk_hosts with context %}
{% set fqdns = clusters.s3meta01.fqdns | join(',') %}
{% set pgmeta_fqdns = clusters.pgmeta01.fqdns | join(',') %}

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
        vm.nr_hugepages: 0
    runlist:
        - components.postgres
        - components.pg-dbs.s3meta
        - components.linux-kernel
        - components.deploy.agent
    connection_pooler: odyssey
    config:
        pgmeta:
            server: {{ pgmeta_fqdns }}
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        effective_cache_size: {{ (clusters.s3meta01.resources.memory * 1024 * 2 / 3 // 1024) | int }}GB
        work_mem: 64MB
        max_prepared_transactions: 10
        log_min_duration_statement: 50ms
        lock_timeout: 200ms
        wal_sender_timeout: 15s
        wal_receiver_timeout: 15s
        default_statistics_target: 1000
    pgsync:
        zk_lockpath_prefix: /pgsync/s3meta01
        zk_hosts: {{ zk_hosts }}
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
            exclude: '"ERRCODE_S3_NoSuchBucket,ERRCODE_S3_BucketAlreadyOwnedByYou"'
        pg_counters_update:
            dbname: s3meta
            warn: 1000
            crit: 3000
        pg_prepared_xacts:
            all: True
        mem_usage:
            check: True
    s3pgmeta: 'host={{ pgmeta_fqdns }} port=6432 dbname=s3db user=s3ro'
    s3meta:
        target: 105
        chunk_mover:
            enable: True
            auto: True
            koeff_chunks_max: 12
        db: 'host=localhost port=6432 dbname=s3meta user=s3util'
        user: 's3util'
        update_chunks_counters:
            threads: 2
    s3_closer:
        pgmeta: 'host={{ pgmeta_fqdns }} port=6432 dbname=s3db'
        db: 'host=localhost dbname=s3meta'
    walg:
        nobalancer: False
        suffix: s3meta01
        parallel_workers: 8
    use_pg_partman: True
    kcache:
        version: '129-a27ce97'
        buffer_size: 1024
        stat_time_interval: 86400
        excluded_keys: 'request_id'
    s3:
       gpg_key: {{ salt.lockbox.get("bcn9j57osi4nf9pqet3q").gpg_key | tojson }}
       gpg_key_id: {{ salt.lockbox.get("bcn9j57osi4nf9pqet3q").gpg_key_id | tojson }}
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
    - mdb_s3_israel.pgusers.pgproxy_common
    - mdb_s3_israel.pgusers.pgproxy_pgpass


{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

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
    auto_resetup: True
    runlist:
        - components.postgres
        - components.pg-dbs.s3meta
        - components.dbaas-cron
        - components.pushclient2
        - components.dbaas-billing
    config:
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        effective_cache_size: 64GB
        work_mem: 64MB
        max_prepared_transactions: 30
        log_min_duration_statement: 50ms
        lock_timeout: 200ms
        max_parallel_workers_per_gather: 0
        max_parallel_workers: 0
        max_client_conn: 40000
        max_client_pool_conn: 16000
        wal_sender_timeout: 15s
        wal_receiver_timeout: 15s
        default_statistics_target: 1000
        pgmeta:
            server: pgmeta-blue01h.db.yandex.net
    pgsync:
        zk_hosts: zkeeper05f.db.yandex.net:2181,zkeeper05h.db.yandex.net:2181,zkeeper05k.db.yandex.net:2181
        priority: {{ priority }}
        zk_lockpath_prefix: '/pgsync/s3meta_blue01'
        remaster_restart: 'no'
    monrun:
        tls:
            warn: 30
        bouncer_log_errors:
            warn: '50\ 50'
            crit: '400\ 400'
        pg_log_errors:
            warn: '10\ 10'
            crit: '100\ 100'
            ignore_read_only: False
            exclude: '"ERRCODE_S3_NoSuchBucket,ERRCODE_S3_BucketAlreadyOwnedByYou,ERRCODE_RAISE_EXCEPTION"'
        pg_counters_update:
            dbname: s3meta
            warn: 4000
            crit: 9000
        pg_prepared_xacts:
            all: True
    pgbouncer:
        count: 8
    s3pgmeta: 'host=pgmeta-blue01h.db.yandex.net port=6432 dbname=s3db user=s3ro'
    s3meta:
        target: 107
        chunk_mover:
            enable: True
            auto: False
            max_threads: 11
            allow_same_shard: True
            one_shard_threshold: 1000000
        db: 'host=localhost port=6432 dbname=s3meta user=s3util'
        user: 's3util'
        update_chunks_counters:
            threads: 5
        sentry_dsn: {{ salt.yav.get('ver-01f5e2zwfq9e1at9y53wbc5bsf[secret]') }}
    s3_closer:
        pgmeta: 'host=pgmeta-blue01h.db.yandex.net port=6432 dbname=s3db'
        db: 'host=localhost dbname=s3meta'
        crit_count: 4
    monrun2: True
    walg:
        storage: ssh
        ssh_server: pg-backup02i.db.yandex.net
        ssh_cluster: s3meta_blue01
        parallel_workers: 8
        backup_election_timeout: 180
    gpg:
        cluster_name: s3meta_blue01
    do_clear_after_repack: True
    use_pg_partman: True
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

include:
    - index_repack
    - porto.prod.selfdns.realm-mdb
    - envs.{{ env }}
    - porto.prod.pgusers.common
    - porto.prod.pgusers.s3_blue
    - porto.prod.pgusers.s3_blue_pgmeta
    - porto.prod.walg
    - mdb_s3_porto_prod.kcache

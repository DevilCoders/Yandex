{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:

    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    pgsync:
        zk_hosts: zkeeper-test02k.db.yandex.net:2181,zkeeper-test02e.db.yandex.net:2181,zkeeper-test02h.db.yandex.net:2181
        remaster_restart: 'no'
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
    condgroup: mdb_s3db_porto_test
    runlist:
        - components.postgres
        - components.pg-dbs.s3db
        - components.dbaas-cron
        - components.pg-dbs.unmanaged.perf-diag-enabled
        - components.pushclient2
        - components.dbaas-billing
    config:
        shared_buffers: 1GB
        lock_timeout: 2s
        wal_log_hints: 'on'
        checkpoint_segments: 10
        wal_keep_segments: 0
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        range_size: 150000000
        pool_mode: transaction
        max_prepared_transactions: 10
        max_standby_streaming_delay: 1s
        wal_sender_timeout: 15s
        wal_receiver_timeout: 15s
        wal_level: logical
        auto_kill_users: "'s3replication'"
    pgbouncer:
        count: 2
        internal_count: 1
        custom_user_pool: True
        custom_user_params:
            - 'sharpei=pool_mode=session'
    sysctl:
        vm.nr_hugepages: 100
    auto_resetup: True
    s3proxy: 'host=pgproxy-test.mail.yandex.net port=6432 dbname=s3db user=s3api'
    s3pgmeta: 'host=pgmeta-test01h.mail.yandex.net,pgmeta-test01i.mail.yandex.net port=6432 dbname=s3db user=s3ro'
    s3db:
        purge_objects:
            ttl: '1 day'
            batch_size: 10000
        update_chunks_counters:
            enable: False
        chunk_splitter:
            threshold: 30000
        copy_delete_queue:
            enable: False
        sentry_dsn: {{ salt.yav.get('ver-01f5e2zwfq9e1at9y53wbc5bsf[secret]') }}
    pgmeta:
        server: pgmeta-test01h.mail.yandex.net
    s3_closer:
        pgmeta: 'host=pgmeta-test01h.mail.yandex.net,pgmeta-test01i.mail.yandex.net port=6432 dbname=s3db'
        db: 'host=localhost dbname=s3db'
    pg-classify:
        monitor: "username:monitor dbname:* group:monitor subsys:cpu"
    use_pg_partman: True
    monrun:
        tls:
            warn: 30
        load_average_relative:
            warn: 2
            crit: 4
        pg_log_errors:
            exclude: '"ERRCODE_S3_NoSuchBucket,ERRCODE_UNIQUE_VIOLATION,ERRCODE_S3_BucketAlreadyOwnedByYou,NOT_KNOWN_ERROR: 67219683"'
            warn: '200\ 200'
        bouncer_log_errors:
            exclude: s3cleanup,s3service
            warn: '500\ 500'
        pg_counters_queue:
            dbname: s3db
            schema: s3
            table: chunks_counters_queue
            warn: 50000
            crit: 500000
        pg_prepared_xacts:
            all: True
    monrun2: True
    l3host: True
    walg:
        storage: ssh
        ssh_server: pgbackup-test01h.db.yandex.net
    kcache:
        version: '138-02f6178'
        buffer_size: 200
        stat_time_interval: 86400
        excluded_keys: 'request_id'
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
    - table_repack
    - envs.{{ env }}
    - porto.test.selfdns.realm-mdb
    - porto.test.walg
    - porto.test.pgusers.common
    - porto.test.pgusers.pgproxy_common
    - porto.test.pgusers.pgproxy_pgpass
    - porto.test.pgusers.s3
    - porto.test.pgusers.pgmeta_s3

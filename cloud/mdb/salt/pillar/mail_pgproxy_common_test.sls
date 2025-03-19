{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    condgroup: mail_pgproxy_common_test
    connection_pooler: pgbouncer
    use_pgsync: False
    use_walg: False
    runlist:
        - components.postgres
        - components.pg-dbs.pgproxy
        - components.network
    diagnostic_tools: False
    pg_ssl_balancer: pgproxy-test.mail.yandex.net
    pgbouncer:
        count: 2
        internal_count: 1
        client_tls_sslmode: require
    pgmeta:
        server: pgmeta-test01h.mail.yandex.net
        read_port: 5432
    monrun:
        pg_replication_alive:
            warn: '-1'
            crit: '-1'
        load_average_relative:
            warn: 2
            crit: 4
        pg_log_errors:
            exclude: S3
    config:
        max_connections: 1024
        work_mem: 1MB
        maintenance_work_mem: 16MB
        bgwriter_delay: 200ms
        bgwriter_lru_maxpages: 100
        checkpoint_segments: 64
        checkpoint_timeout: 5min
        checkpoint_completion_target: 0.5
        checkpoint_warning: 30s
        shared_preload_libraries: pg_stat_statements,repl_mon
        log_hostname: 'off'
        log_lock_waits: 'off'
        shared_buffers: 512MB
        archive_mode: 'off'
        pool_mode: statement
        databases:
            - rpopdb
            - rpopdb_grascm
            - s3db
        db_users_map:
            rpopdb_grascm:
                user:
                    rpop
                code:
                    - pgproxy/rpopdb.sql
    sysctl:
        vm.nr_hugepages: 0
    yasmagent:
        prj_split_by_shard: False
    monrun2: True
    l3host: True
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8::3400:0:55d:0:1
{% set edition = 'default' %}
{% set pg_major_version = 10 %}
{% set env = 'dev' %}
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
    - envs.{{ env }}
    - porto.test.selfdns.realm-sandbox
    - porto.test.pgusers.common
    - porto.test.pgusers.pgproxy_common
    - porto.test.pgusers.rpop
    - porto.test.pgusers.rpop_dev
    - porto.test.pgusers.s3

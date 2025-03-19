{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    condgroup: mail_pgtest_pgproxy
    connection_pooler: pgbouncer
    use_pgsync: False
    use_walg: False
    runlist:
        - components.postgres
        - components.pg-dbs.pgproxy
    diagnostic_tools: False
    pgmeta:
        server: pgmeta-load01h.mail.yandex.net
        read_port: 6432
    monrun:
        pg_replication_alive:
            warn: '-1'
            crit: '-1'
    config:
        atop_rotate_days: 4
        shared_buffers: 128MB
        shared_preload_libraries: pg_stat_statements,repl_mon
        archive_mode: off
        pool_mode: statement
        databases:
            - s3db
    sysctl:
        vm.nr_hugepages: 0
    monrun2: True
    pgbouncer:
        count: 4
        internal_count: 1
        client_tls_sslmode: require
{% set edition = 'default' %}
{% set pg_major_version = 10 %}
{% set env = 'qa' %}
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
    - envs.load
    - porto.prod.pgusers.load.common
    - porto.prod.pgusers.load.pgproxy_common
    - porto.prod.pgusers.load.s3

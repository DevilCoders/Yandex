{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    auto_resetup: True
    condgroup: mail_pgtest_pgmeta
    monrun2: True
    monrun:
        tls:
            warn: 30
        pg_replication_alive:
            warn: -1
            crit: -1
    runlist:
        - components.postgres
        - components.pg-dbs.pgmeta
    pgsync:
        zk_hosts: zkeeper-test01e.db.yandex.net:2181,zkeeper-test01f.db.yandex.net:2181,zkeeper-test01h.db.yandex.net:2181
    config:
        shared_buffers: 128MB
        pool_mode: transaction
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        wal_log_hints: 'on'
        pg_hba:
            - 'host all pgproxy 0.0.0.0/0 md5'
            - 'host all pgproxy ::/0 md5'
        databases:
            - rpopdb
            - rpopdb_grascm
            - rpopdb_egorp
            - s3db
    db_code_map:
        s3db: s3db.sql
    sysctl:
        vm.nr_hugepages: 0
{% set env = 'dev' %}
{% set pg_major_version = 10 %}
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
    - envs.dev
    - porto.prod.pgusers.dev.common
    - porto.prod.pgusers.dev.pgproxy_common
    - porto.prod.pgusers.dev.pgproxy
    - porto.prod.pgusers.dev.admin_pgpass
    - porto.prod.pgusers.dev.pgmeta_s3
    - porto.prod.s3.pgaas_s3backup
    - porto.prod.selfdns.realm-sandbox

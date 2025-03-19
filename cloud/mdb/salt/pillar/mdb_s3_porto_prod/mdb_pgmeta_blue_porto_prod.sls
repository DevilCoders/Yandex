{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    condgroup: mdb_pgmeta_blue_porto_prod
    runlist:
        - components.postgres
        - components.pg-dbs.pgmeta
    ipv6selfdns: True
    config:
        pgbouncer_redirect: True
        pool_mode: transaction
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        wal_log_hints: 'on'
        pg_hba:
            - host    all             pgproxy         ::/0                    md5
        databases:
            - s3db
        atop_rotate_days: 7
    db_code_map:
        s3db: s3db.sql
    sysctl:
        vm.nr_hugepages: 2500
    walg:
        parallel_workers: 8
    pgsync:
        zk_hosts: zkeeper05f.db.yandex.net:2181,zkeeper05h.db.yandex.net:2181,zkeeper05k.db.yandex.net:2181 
        overload_sessions_ratio: 10
        replica_remaster_checks: 3
    monrun2: True
    monrun:
        tls:
            warn: 30
        pg_log_errors:
            warn: '10\ 50'
{% set edition = 'default' %}
{% set pg_major_version = 12 %}
{% set env = 'prod' %}
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
    - porto.prod.pgusers.common
    - porto.prod.pgusers.s3_blue_pgmeta
    - porto.prod.selfdns.realm-mdb
    - porto.prod.s3.pgaas_s3backup

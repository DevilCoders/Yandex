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
    runlist:
        - components.postgres
        - components.pg-dbs.dbm
    config:
        shared_buffers: 1GB
        max_connections: 1024
        lock_timeout: 60s
        wal_log_hints: 'on'
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        autovacuum_vacuum_cost_delay: 20ms
        autovacuum_vacuum_cost_limit: 1000
        pool_mode: transaction
    sysctl:
        vm.nr_hugepages: 0
    pgsync:
        zk_hosts: zkeeper01e.db.yandex.net:2181,zkeeper01f.db.yandex.net:2181,zkeeper01h.db.yandex.net:2181
    cauth_use_v2: True
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: 'mdb_dbm'
        service: mdb

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

include:
    - envs.{{ env }}
    - porto.prod.dbaas.solomon
    - porto.prod.selfdns.realm-mdb
    - porto.prod.s3.pgaas_s3backup
    - porto.prod.pgusers.common
    - porto.prod.pgusers.dbm
    - porto.prod.pgusers.mdb_ui
    - index_repack

{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    l3host: True
    monrun2: True
    network_autoconf: True
    runlist:
        - components.common
        - components.postgres
        - components.repositories.apt.pgdg
        - components.repositories.apt.yandex-postgresql.unstable
        - components.repositories.apt.yandex-postgresql.testing
        - components.repositories.apt.yandex-postgresql.stable
    config:
        server_reset_query_always: 1
        shared_preload_libraries: repl_mon
        archive_mode: 'off'
        pool_mode: transaction
        pg_hba:
            - host all monitor ::/0 md5
    use_pgsync: False
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
    connection_pooler: odyssey
    pgbouncer:
        count: 4
        internal_count: 4
        log_connections: 0
        log_disconnections: 0
include:
    - envs.{{ env }}
    - porto.prod.pgusers.dev.common
    - porto.prod.selfdns.realm-mail

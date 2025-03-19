{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    runlist:
        - components.postgres
    connection_pooler: pgbouncer
    use_walg: False
    use_pgsync: False
    pgsync:
        min_failover_timeout: 60
    config:
        shared_buffers: 128MB
        wal_level: logical
        wal_keep_segments: 64
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
    sysctl:
        vm.nr_hugepages: 70
    monrun:
        pgsync:
            failover: 180
    monrun2: True
{% set env = 'dev' %}
{% set pg_major_version = 11 %}
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
    - envs.{{ env }}
    - porto.prod.pgusers.dev.common
    - porto.prod.selfdns.realm-sandbox
    - porto.prod.s3.pgaas_s3backup

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
        - components.pg-dbs.mlockdb
        - components.dbaas-porto-controlplane
    mlockdb:
        target: latest
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    config:
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
    pgsync:
        zk_hosts: zkeeper01e.db.yandex.net:2181,zkeeper01f.db.yandex.net:2181,zkeeper01h.db.yandex.net:2181
    use_yasmagent: True
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
    - mdb_controlplane_porto_prod.common
    - porto.prod.pgusers.common
    - porto.prod.pgusers.mlock
    - porto.prod.pgusers.mlockdb_admin
    - porto.prod.pgusers.mdb_ui
    - porto.prod.s3.pgaas_s3backup

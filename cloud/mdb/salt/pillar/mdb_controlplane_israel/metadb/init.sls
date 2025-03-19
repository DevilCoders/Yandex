{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}
{% from "mdb_controlplane_israel/map.jinja" import clusters with context %}

mine_functions:
    grains.item:
        - id
        - role
        - pg
        - virtual

data:
    runlist:
        - components.postgres
        - components.pg-dbs.dbaas_metadb
        - components.dbaas-compute-controlplane
        - components.deploy.agent
    config:
        shared_buffers: {{ clusters.metadb.resources.memory // 4 }}GB
        maintenance_work_mem: 515MB
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        min_wal_size: 1GB
        max_wal_size: 1GB
        log_min_duration_statement: 100ms
        lock_timeout: 10s
        default_statistics_target: 1000
        wal_level: logical
        pgusers:
{% for user, lockbox in clusters.metadb.users.items() %}
            {{ user }}:
                password: {{ salt.lockbox.get(lockbox.secret_id)[lockbox.secret_key] }}
{% endfor %}

{% set env = 'compute-prod' %}
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
    s3_bucket: {{ clusters.metadb.backups_bucket }}
    dbaas_metadb:
        enable_cleaner: False
        valid_resources_env: israel_prod
    pgsync:
        zk_lockpath_prefix: metadb

include:
    - envs.{{ env }}
    - mdb_controlplane_israel.common
    - mdb_controlplane_israel.common.pg
    - mdb_controlplane_israel.metadb.gpg
{% for user in clusters.metadb.users %}
    - mdb_controlplane_israel.pgusers.{{ user }}
{% endfor %}

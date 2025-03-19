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
        - components.pg-dbs.deploydb
        - components.dbaas-compute-controlplane
        - components.deploy.agent
    config:
        shared_buffers: {{ clusters.deploydb.resources.memory // 4 }}GB
        maintenance_work_mem: 512MB
        max_connections: 400
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        lock_timeout: 60s
        pgusers:
{% for user, lockbox in clusters.deploydb.users.items() %}
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
    s3_bucket: {{ clusters.deploydb.backups_bucket }}
    pgsync:
        zk_lockpath_prefix: deploydb

include:
    - envs.{{ env }}
    - mdb_controlplane_israel.common
    - mdb_controlplane_israel.common.pg
    - mdb_controlplane_israel.deploydb.gpg
    - mdb_deploydb_common
{% for user in clusters.deploydb.users %}
    - mdb_controlplane_israel.pgusers.{{ user }}
{% endfor %}

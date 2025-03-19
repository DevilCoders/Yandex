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
        - components.pg-dbs.secretsdb
        - components.dbaas-compute-controlplane
        - components.deploy.agent
    config:
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        shared_buffers: {{ clusters.secretsdb.resources.memory // 4 }}GB
        maintenance_work_mem: 512MB
        archive_timeout: 10min
        max_connections: 400
        pgusers:
{% for user, lockbox in clusters.secretsdb.users.items() %}
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
    s3_bucket: {{ clusters.secretsdb.backups_bucket }}
    pgsync:
        zk_lockpath_prefix: secretsdb

include:
    - envs.{{ env }}
    - mdb_controlplane_israel.common
    - mdb_controlplane_israel.common.pg
    - mdb_controlplane_israel.secretsdb.gpg
{% for user in clusters.secretsdb.users %}
    - mdb_controlplane_israel.pgusers.{{ user }}
{% endfor %}

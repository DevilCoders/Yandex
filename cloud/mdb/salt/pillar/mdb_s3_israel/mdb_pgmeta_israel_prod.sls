{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}
{% from "mdb_s3_israel/map.jinja" import zk_hosts with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
{% set env = 'compute-prod' %}
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
    sysctl:
        vm.nr_hugepages: 0
    runlist:
        - components.postgres
        - components.pg-dbs.pgmeta
        - components.firewall
        - components.linux-kernel
        - components.deploy.agent
    connection_pooler: odyssey
    config:
        pg_hba:
            - host    all             pgproxy         ::/0                    md5
        shared_buffers: 1GB
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        databases:
            - s3db
    db_code_map:
        s3db: s3db.sql
    pgsync:
        zk_lockpath_prefix: /pgsync/pgmeta01
        zk_hosts: {{ zk_hosts }}
    monrun:
        tls:
            warn: 30
    walg:
        nobalancer: False
        suffix: pgmeta01
    s3:
       gpg_key: {{ salt.lockbox.get("bcni67qshd6p6quj3ftg").gpg_key | tojson }}
       gpg_key_id: {{ salt.lockbox.get("bcni67qshd6p6quj3ftg").gpg_key_id | tojson }}
       host: storage-internal.cloud.yandex.net
       access_key_id: {{ salt.lockbox.get("bcn929e77d82gj4k5i8h").access_key_id | tojson }}
       access_secret_key: {{ salt.lockbox.get("bcn929e77d82gj4k5i8h").access_secret_key | tojson }}
       endpoint: https+path://storage-internal.cloud.yandex.net
    s3_bucket: mdb-s3-israel

firewall:
    policy: ACCEPT
    ACCEPT:
      - net: 2a11:f740:1:0:9000:1d::/96
        type: addr6
        ports:
          - '5432'
    REJECT:
      - net: '::/0'
        type: addr6
        ports:
          - '5432'
      - net: '0/0'
        type: addr4
        ports:
          - '5432'

include:
    - compute_vtype
    - index_repack
    - envs.{{ env }}
    - mdb_s3_israel.common
    - mdb_s3_israel.pgusers.common
    - mdb_s3_israel.pgusers.pgproxy_common
    - mdb_s3_israel.pgusers.pgproxy
    - mdb_s3_israel.pgusers.pgmeta_s3


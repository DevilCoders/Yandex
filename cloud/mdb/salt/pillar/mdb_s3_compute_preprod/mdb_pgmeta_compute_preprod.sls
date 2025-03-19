{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
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
    sysctl:
        vm.nr_hugepages: 0
    condgroup: mdb_pgmeta_compute_preprod
    runlist:
        - components.postgres
        - components.pg-dbs.pgmeta
        - components.firewall
        - components.linux-kernel
        - components.dbaas-compute.read-ahead
        - components.dbaas-compute.underlay
    cauth_use: False
    ipv6selfdns: True
    connection_pooler: odyssey
    dbaas_compute:
        read_ahead_kb: 0
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
        zk_hosts: zk-s3-compute01f.svc.cloud-preprod.yandex.net:2181,zk-s3-compute01h.svc.cloud-preprod.yandex.net:2181,zk-s3-compute01k.svc.cloud-preprod.yandex.net:2181
    mdb_metrics:
        enabled: True
        system_sensors: True
    use_yasmagent: False
    monrun:
        tls:
            warn: 30
    monrun2: True
    walg:
        nobalancer: False
        suffix: pgmeta01-compute-preprod

firewall:
    policy: ACCEPT
    ACCEPT:
      - net: _C_MDB_PGMETA_COMPUTE_PREPROD_
        type: macro
        ports:
          - '5432'
      - net: _C_MDB_S3META_COMPUTE_PREPROD_
        type: macro
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
    - compute.preprod.s3.solomon
    - compute.preprod.selfdns.realm-cloud-mdb
    - compute.preprod.pgusers.common
    - compute.preprod.pgusers.pgmeta_s3
    - compute.preprod.s3.pgaas_s3backup

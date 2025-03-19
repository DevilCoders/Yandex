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
        - components.pg-dbs.cmsdb
        - components.dbaas-compute-controlplane
    cmsdb:
        target: 23
    pgsync:
        zk_hosts: zk-dbaas01f.yandexcloud.net:2181,zk-dbaas01h.yandexcloud.net:2181,zk-dbaas01k.yandexcloud.net:2181
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
    ipv6selfdns: True
    sysctl:
        vm.nr_hugepages: 0
    config:
        shared_buffers: 1GB
        maintenance_work_mem: 512MB
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        min_wal_size: 1GB
        max_wal_size: 1GB
    dbaas:
        vtype: compute
    cauth_use: False
    solomon:
        cluster: mdb_cms_db_compute_prod
    use_yasmagent: True
    s3_bucket: yandexcloud-dbaas-cmsdb-prod

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.solomon
    - mdb_controlplane_compute_prod.common.deploy
    - compute.prod.s3.mdb_s3backup
    - compute.prod.pgusers.common
    - compute.prod.pgusers.cms
    - compute.prod.pgusers.cmsdb_admin
    - compute.prod.pgusers.mdb_ui

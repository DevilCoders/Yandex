{% from "map.jinja" import pg_versions_package_map, odyssey_versions_package_map with context %}

mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    migrates_in_k8s: True
    auto_resetup: True
    runlist:
        - components.postgres
        - components.pg-dbs.cmsdb
        - components.dbaas-compute-controlplane
    cmsdb:
        target: latest
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    pgsync:
        zk_hosts: zk-dbaas-preprod01f.cloud-preprod.yandex.net:2181,zk-dbaas-preprod01h.cloud-preprod.yandex.net:2181,zk-dbaas-preprod01k.cloud-preprod.yandex.net:2181
{% set env = 'qa' %}
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
        cluster: 'mdb_{ctype}'
    use_yasmagent: True
    s3_bucket: yandexcloud-dbaas-cmsdb-preprod

include:
    - envs.{{ env }}
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - compute.preprod.s3.mdb_s3backup
    - compute.preprod.pgusers.common
    - compute.preprod.pgusers.cms
    - compute.preprod.pgusers.cmsdb_admin
    - compute.preprod.pgusers.mdb_ui

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
        - components.pg-dbs.deploydb
        - components.dbaas-compute-controlplane
    deploydb:
        target: latest
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    sysctl:
        vm.nr_hugepages: 0
    config:
        shared_buffers: 1GB
        maintenance_work_mem: 512MB
        max_connections: 400
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        lock_timeout: 60s
    pgsync:
        zk_hosts: zk-dbaas-preprod01f.cloud-preprod.yandex.net:2181,zk-dbaas-preprod01h.cloud-preprod.yandex.net:2181,zk-dbaas-preprod01k.cloud-preprod.yandex.net:2181
        failover_checks: 60
    ipv6selfdns: True
    monrun2: True
    use_yasmagent: True
    solomon:
        cluster: mdb_deploy_db_compute_preprod
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
    dbaas:
        vtype: compute
    cauth_use: False
    s3_bucket: yandexcloud-dbaas-deploydb

include:
    - envs.{{ env }}
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - compute.preprod.pgusers.common
    - compute.preprod.pgusers.deploy_api
    - compute.preprod.pgusers.deploy_cleaner
    - compute.preprod.pgusers.mdb_ui
    - compute.preprod.s3.mdb_s3backup
    - compute.preprod.pgusers.deploydb_admin
    - mdb_deploydb_common

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
        - components.pg-dbs.deploydb
        - components.dbaas-porto-controlplane
    deploydb:
        target: latest
    deploy:
        version: 2
        api_host: deploy-api-test.db.yandex-team.ru
    config:
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
        archive_timeout: 10min
        lock_timeout: 60s
    pgsync:
        zk_hosts: zkeeper-test01e.db.yandex.net:2181,zkeeper-test01h.db.yandex.net:2181,zkeeper-test01f.db.yandex.net:2181
        failover_checks: 60
    monrun2: True
    use_yasmagent: True
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_deploy_db_porto_test
        service: deploy_db
{% set env = 'dev' %}
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
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.selfdns
    - porto.test.pgusers.common
    - porto.test.pgusers.deploy_api
    - porto.test.pgusers.deploy_cleaner
    - porto.test.pgusers.deploydb_admin
    - porto.test.pgusers.mdb_ui
    - porto.test.s3.pgaas_s3backup
    - porto.test.dbaas.solomon
    - mdb_deploydb_common

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
        - components.pg-dbs.dbm
        - components.web-api-base
        - components.db-maintenance
        - components.jaeger-agent
        - components.network
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.mdb-controlplane-telegraf
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:1
    l3host: True
    ipv6selfdns: True
    monrun2: True
    pg_ssl_balancer: {{ salt.grains.get('id') }}
    pg_ssl_balancer_alt_names:
        - mdb-test.db.yandex-team.ru
    config:
        shared_preload_libraries: pg_stat_statements,pg_stat_kcache,repl_mon
    sysctl:
        vm.nr_hugepages: 0
    pgsync:
        zk_hosts: zkeeper05f.db.yandex.net:2181,zkeeper05h.db.yandex.net:2181,zkeeper05k.db.yandex.net:2181
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: 'mdb_dbm_test'
        service: mdb
    sentry:
        environment: porto-test
    cauth_use_v2: True
    dbm:
        old_backups_threshold: 7200
        base_url: https://mdb-test.db.yandex-team.ru/
    db_maintenance:
        sentry:
            dsn: '{{ salt.yav.get('ver-01fdye579psz5jjh4p4c4kt8z2[dsn]') }}'
        base_host: mdb-test.db.yandex-team.ru
        db_hosts:
            - dbm-test01f.db.yandex.net
            - dbm-test01h.db.yandex.net
            - dbm-test01k.db.yandex.net
        deploy_api_v2_url: https://deploy-api-test.db.yandex-team.ru/
        deploy_env: portotest
        salt_env: dev
        reserved_resources:
            '1':
                cpu: 0
                memory: 0
                io: 0
                net: 0
            '2':
                cpu: 0
                memory: 0
                io: 0
                net: 0
            '3':
                cpu: 0
                memory: 0
                io: 0
                net: 0
        allow_logins:
            - robot-mdb-cms-porto
            - arhipov
            - d0uble
            - dsarafan
            - efimkin
            - pperekalov
            - secwall
            - x4mmm
            - alex-burmak
            - wizard
            - sidh
            - schizophrenia
            - mialinx
            - annkpx
            - teem0n
            - reshke
            - denchick
            - ein-krebs
            - vlbel
            - pervakovg
            - khattu
            - frisbeeman
            - ayasensky
            - iantonspb
            - kashinav
            - usernamedt
            - irioth
            - peevo
            - velom
            - estrizhnev
            - waleriya
            - dstaroff
            - vadim-volodin
            - rivkind-nv
            - vorobyov-as
            - stewie
            - andrei-vlg
            - robot-pgaas-deploy
            - fedusia
            - sageneralova
            - andrei-vlg
            - robert
            - yurovniki
            - moukavoztchik
            - prez
            - dkhurtin
            - vicpopov
            - yoschi
            - vgoshev
            - roman-chulkov
            - mariadyuldina
            - eshkrig
            - xifos
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
    - porto.prod.dbaas.solomon
    - porto.prod.selfdns.realm-mdb
    - porto.prod.s3.pgaas_s3backup
    - porto.prod.pgusers.dev.common
    - porto.prod.pgusers.dev.dbm
    - porto.test.pgusers.mdb_ui
    - porto.prod.dbm.mdb_test_db_yandex_team_ru
    - porto.test.dbaas.jaeger_agent
    - index_repack

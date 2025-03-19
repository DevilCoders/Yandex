mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    runlist:
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
                - 2a02:6b8:0:3400:0:a64:0:c
    l3host: True
    ipv6selfdns: True
    monrun2: True
    pg_ssl_balancer: mdb.yandex-team.ru
    cert:
        server_name: mdb.yandex-team.ru
    sentry:
        environment: porto-prod
    cauth_use_v2: True
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: 'mdb_dbm'
        service: mdb
    db_maintenance:
        skip_overcommit_regexes:
            - '"^man.*"'
            - '".*01i.db.yandex.net"'
        sentry:
            dsn: '{{ salt.yav.get('ver-01fe0t78cgbev3qgqp20cxxxfd[dsn]') }}'
        base_host: mdb.yandex-team.ru
        db_hosts:
            - dbmdb01f.db.yandex.net
            - dbmdb01h.db.yandex.net
            - dbmdb01k.db.yandex.net
        deploy_api_v2_url: https://deploy-api.db.yandex-team.ru/
        deploy_env: portoprod
        allow_logins:
            - robot-mdb-cms-porto
            - khattu
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
            - vgoshev
            - ignition
            - ivanov-d-s
            - robot-pgaas-deploy
            - reshke
            - denchick
            - ein-krebs
            - peevo
            - vlbel
            - munakoiso
            - pervakovg
            - frisbeeman
            - ayasensky
            - amatol
            - iantonspb
            - kashinav
            - usernamedt
            - irioth
            - velom
            - estrizhnev
            - waleriya
            - dstaroff
            - vadim-volodin
            - knyazzhev
            - rivkind-nv
            - vorobyov-as
            - stewie
            - teem0n
            - fedusia
            - sageneralova
            - andrei-vlg
            - robert
            - yurovniki
            - moukavoztchik
            - dkhurtin
            - vicpopov
            - egor-medvedev
            - roman-chulkov
            - mariadyuldina
            - eshkrig
            - xifos
            - yoschi

include:
    - envs.prod
    - porto.prod.selfdns.realm-mdb
    - porto.prod.dbaas.dbm
    - porto.prod.pgusers.dbm
    - porto.prod.dbaas.jaeger_agent
    - porto.prod.dbaas.solomon

mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.nginx
        - components.mdb_cms
        - components.jaeger-agent
        - components.monrun2
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.dbaas-porto-controlplane
    monrun2: True
    use_yasmagent: True
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_cms_porto_prod
        service: mdb
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    mdb_cms:
        conductor:
            oauth: {{ salt.yav.get('ver-01fsrzc3ngp9p8gr3r7t2fs7ys[conductor]') }}
        mlock:
            host: mlock.db.yandex.net:443
        service_account:
            id: f6o1vvi8tohnt5uhjbp1
            key_id: {{ salt.yav.get('ver-01erxxkr406cxbrdff2crebbxg[key_id]') }}
            private_key: {{ salt.yav.get('ver-01erxxkr406cxbrdff2crebbxg[private_key]') | yaml_encode }}
        sentry:
            dsn: {{ salt.yav.get('ver-01e7qd9pnafc84ch49nnjxrkqg[dsn]') }}
        instrumentation:
            port: 6060
        juggler:
            oauth: {{ salt.yav.get('ver-01ed26xft2k0kvy20mtqdbcxdw[juggler_oauth]') }}
        dbm:
            token: {{ salt.yav.get('ver-01e8m5zx0e9vjpy59wgq3n2699[dbm_token]') }}
            host: mdb.yandex-team.ru
        new_requests_are_not_important: 0
        deploy:
            token: {{ salt.yav.get('ver-01ef92vh0qyjk917xxk50591bs[deploy_oauth]') }}
            hostname: deploy-api.db.yandex-team.ru
        health:
            host: health.db.yandex.net
        cms:
            steps:
                deploy_minion_group: porto-prod
                conductor_dom0_group: 24228  # mdb_dom0
                drills:
                  - ticket: DRILLS-682
                    availability_zones:
                    - myt
                    from: 2022-12-13T15:00:00+03:00
                    till: 2022-12-13T16:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-681
                    availability_zones:
                    - vla
                    from: 2022-11-29T09:00:00+03:00
                    till: 2022-11-29T20:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-680
                    availability_zones:
                    - iva
                    from: 2022-11-15T15:00:00+03:00
                    till: 2022-11-15T16:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-679
                    availability_zones:
                    - man
                    from: 2022-11-01T15:00:00+03:00
                    till: 2022-11-01T16:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-678
                    availability_zones:
                    - sas
                    from: 2022-10-18T10:00:00+03:00
                    till: 2022-10-18T19:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-677
                    availability_zones:
                    - myt
                    from: 2022-10-04T15:00:00+03:00
                    till: 2022-10-04T16:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-676
                    availability_zones:
                    - vla
                    from: 2022-09-20T09:00:00+03:00
                    till: 2022-09-20T20:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-675
                    availability_zones:
                    - iva
                    from: 2022-09-06T15:00:00+03:00
                    till: 2022-09-06T16:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-673
                    availability_zones:
                    - sas
                    from: 2022-08-09T10:00:00+03:00
                    till: 2022-08-09T19:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-672
                    availability_zones:
                    - myt
                    from: 2022-07-26T15:00:00+03:00
                    till: 2022-07-26T16:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-670
                    availability_zones:
                    - iva
                    from: 2022-06-28T15:00:00+03:00
                    till: 2022-06-28T16:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-669
                    availability_zones:
                    - man
                    from: 2022-06-14T15:00:00+03:00
                    till: 2022-06-14T16:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-668
                    availability_zones:
                    - sas
                    from: 2022-05-31T09:00:00+03:00
                    till: 2022-05-31T19:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-667
                    availability_zones:
                    - myt
                    from: 2022-05-17T15:00:00+03:00
                    till: 2022-05-17T16:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - ticket: DRILLS-666
                    availability_zones:
                    - vla
                    from: 2022-04-26T09:00:00+03:00
                    till: 2022-04-26T20:00:00+03:00
                    wait_before: 18h
                    wait_after: 1h
                  - from: 2022-04-22T15:00:00+03:00
                    till: 2022-04-22T22:00:00+03:00
                    availability_zones:
                    - man
                    ticket: DRILLS-784
                    wait_before: 30h
                    wait_after: 1h
                  - from: 2022-04-13T12:00:00+03:00
                    till: 2022-04-13T19:30:00+03:00
                    availability_zones:
                    - man
                    ticket: DRILLS-775
                    wait_before: 72h
                    wait_after: 1h
                  - from: 2022-04-11T11:00:00+03:00
                    till: 2022-04-11T19:00:00+03:00
                    availability_zones:
                    - man
                    ticket: DRILLS-770
                    wait_before: 72h
                    wait_after: 1h

    tvmtool:
        port: 50001
        tvm_id: 2019557
        config:
            client: mdb-cms-porto-prod
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:9
    pg_ssl_balancer: mdb-cms.db.yandex.net

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - mdb_controlplane_porto_prod.common.groups_lists
    - mdb_controlplane_porto_prod.common.cms
    - mdb_controlplane_porto_prod.common.ui
    - porto.prod.dbaas.mdb_cms
    - porto.prod.dbaas.solomon
    - porto.prod.dbaas.jaeger_agent

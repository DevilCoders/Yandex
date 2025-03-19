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
        cluster: mdb_cms_porto_test
        service: mdb
    deploy:
        version: 2
        api_host: deploy-api-test.db.yandex-team.ru
    mdb_cms:
        conductor:
            oauth: {{ salt.yav.get('ver-01fsrzc3ngp9p8gr3r7t2fs7ys[conductor]') }}
        mlock:
            host: mlock-test.db.yandex.net:443
        service_account:
            id: f6o1vvi8tohnt5uhjbp1
            key_id: {{ salt.yav.get('ver-01erxxkr406cxbrdff2crebbxg[key_id]') }}
            private_key: {{ salt.yav.get('ver-01erxxkr406cxbrdff2crebbxg[private_key]') | yaml_encode }}
        new_requests_are_not_important: 1
        sentry:
            dsn: {{ salt.yav.get('ver-01e7qd1v6g2vjwsq7aactmkgzc[dsn]') }}
        instrumentation:
            port: 6060
        dbm:
            token: {{ salt.yav.get('ver-01e8m5zx0e9vjpy59wgq3n2699[dbm_token]') }}
            host: mdb-test.db.yandex-team.ru
        juggler:
            oauth: {{ salt.yav.get('ver-01ed26xft2k0kvy20mtqdbcxdw[juggler_oauth]') }}
        deploy:
            token: {{ salt.yav.get('ver-01ef92vh0qyjk917xxk50591bs[deploy_oauth]') }}
            hostname: deploy-api-test.db.yandex-team.ru
        health:
            host: mdb-health-test.db.yandex.net
        cms:
            steps:
                deploy_minion_group: porto-test
                conductor_dom0_group: 129559  # mdb_dom0_test
                drills:
                    - from: 2020-10-10T14:00:00+03:00 # example
                      till: 2020-10-10T15:00:00+03:00
                      availability_zones:
                        - zone1
                      ticket: MDB-9355
                      wait_before: 2h
                      wait_after: 1h
                    - from: 2021-01-31T21:00:00+03:00 # test drills
                      till: 2021-01-31T22:00:00+03:00
                      availability_zones:
                        - myt
                      ticket: MDB-9355
                      wait_before: 336h
                      wait_after: 1h
                    - from: 2022-01-18T15:00:00+03:00
                      till: 2022-01-18T16:00:00+03:00
                      availability_zones:
                        - man
                      ticket: DRILLS-648
                      wait_before: 18h
                      wait_after: 1h
                    - from: 2022-02-24T09:00:00+03:00
                      till: 2022-02-24T20:00:00+03:00
                      availability_zones:
                        - vla
                      ticket: DRILLS-662
                      wait_before: 40h
                      wait_after: 1h
                    - from: 2022-04-22T15:00:00+03:00
                      till: 2022-04-22T22:00:00+03:00
                      availability_zones:
                        - man
                      ticket: DRILLS-784
                      wait_before: 30h
                      wait_after: 1h
    tvmtool:
        port: 50001
        tvm_id: 2019397
        config:
            client: mdb-cms-test
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:8
    pg_ssl_balancer: mdb-cms-test.db.yandex.net

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.groups_lists
    - mdb_controlplane_porto_test.common.selfdns
    - mdb_controlplane_porto_test.common.cms
    - mdb_controlplane_porto_test.common.ui
    - porto.test.dbaas.mdb_cms
    - porto.test.dbaas.solomon
    - porto.test.dbaas.jaeger_agent

mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.nginx
        - components.supervisor
        - components.mdb-dns
        - components.jaeger-agent
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.dbaas-porto-controlplane
    monrun2: True
    deploy:
        version: 2
        api_host: deploy-api-test.db.yandex-team.ru
    dbaas:
        shard_hosts:
            - mdb-dns-test01h.db.yandex.net
            - mdb-dns-test01f.db.yandex.net
            - mdb-dns-test01k.db.yandex.net
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:767:0:2
    yasmagent:
        instances:
            - mdbdns
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    pg_ssl_balancer: mdb-dns-test.db.yandex.net
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_dns_porto_test
        service: mdb

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.selfdns
    - porto.test.dbaas.mdb_dns
    - porto.test.dbaas.solomon
    - porto.test.dbaas.jaeger_agent

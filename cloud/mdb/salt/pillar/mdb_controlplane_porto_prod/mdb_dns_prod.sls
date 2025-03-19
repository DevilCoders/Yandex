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
        - components.yasmagent
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.dbaas-porto-controlplane
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    monrun2: True
    dbaas:
        shard_hosts:
            - mdb-dns01h.db.yandex.net
            - mdb-dns01i.db.yandex.net
            - mdb-dns01k.db.yandex.net
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:767:0:9
    pg_ssl_balancer: mdb-dns.db.yandex.net
    yasmagent:
        instances:
            - mdbdns
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_dns_porto_prod
        service: mdb

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - porto.prod.dbaas.mdb_dns
    - porto.prod.dbaas.solomon
    - porto.prod.dbaas.jaeger_agent

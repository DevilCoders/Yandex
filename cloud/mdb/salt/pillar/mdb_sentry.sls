mine_functions:
    grains.item:
        - id
        - role
        - ya
        - virtual
        - mdb_redis

include:
    - envs.dev
    - porto.prod.selfdns.realm-sandbox
    - porto.prod.dbaas.sentry_server
    - mdb_metrics.solomon_intra

data:
    monrun2: True
    use_unbound_64: True
    runlist:
        - components.sentry
        - components.network
    l3host: True
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:767:0:3
    pg_ssl_balancer: sentry.db.yandex-team.ru
    use_mdbsecrets: True
    use_yasmagent: False
    sentry:
        database:
            name: dbaas_sentrydb
            user: sentry
            host: c-f63db1b5-2382-4b55-91cb-67c9a6eed033.rw.db.yandex.net
            port: 6432
        cache:
            host: c-mdbj025npqq9eflsvgcs.rw.db.yandex.net
            port: 6379
        s3:
            url: https://s3.mds.yandex.net
            bucket: dbaas-sentry 

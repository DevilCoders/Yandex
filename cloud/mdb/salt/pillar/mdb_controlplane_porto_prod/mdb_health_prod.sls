mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.redis
        - components.nginx
        - components.supervisor
        - components.mdb-health
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
            - health01h.db.yandex.net
            - health01k.db.yandex.net
            - health01f.db.yandex.net
    redis:
        config:
            loglevel: notice
    sentinel:
        master_name: mymaster
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:767:0:8
    pg_ssl_balancer: health.db.yandex.net
    yasmagent:
        instances:
            - mdbhealth
    mdb_metrics:
        use_yasmagent: False
        main:
            yasm_tags_cmd: /usr/local/yasmagent/mdbhealth_getter.py
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_health_porto_prod
        service: mdb
    common:
        nginx:
            worker_connections: 8192
    walg:
        enabled: False

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - porto.prod.dbaas.mdb_health
    - porto.prod.dbaas.jaeger_agent
    - porto.prod.dbaas.solomon
    - porto.flavor


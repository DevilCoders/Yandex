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
    monrun2: True
    deploy:
        version: 2
        api_host: deploy-api-test.db.yandex-team.ru
    dbaas:
        shard_hosts:
            - health-test01h.db.yandex.net
            - health-test01k.db.yandex.net
            - health-test01f.db.yandex.net
    redis:
        config:
            loglevel: notice
    sentinel:
        master_name: mymaster
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:767:0:5
    pg_ssl_balancer: mdb-health-test.db.yandex.net
    yasmagent:
        instances:
            - mdbhealth
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/mdbhealth_getter.py
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_health_porto_test
        service: mdb
    common:
        nginx:
            worker_connections: 8192
    walg:
        enabled: False

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.selfdns
    - porto.test.dbaas.mdb_health
    - porto.test.dbaas.jaeger_agent
    - porto.test.dbaas.solomon
    - porto.flavor


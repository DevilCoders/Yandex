mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
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
        - components.dbaas-compute-controlplane
    dbaas:
        shard_hosts:
            - health-dbaas01f.yandexcloud.net
            - health-dbaas01h.yandexcloud.net
            - health-dbaas01k.yandexcloud.net
        cluster_name: mdb-health-compute-prod
        vtype: compute
    redis:
        config:
            loglevel: notice
    yasmagent:
        instances:
            - mdbhealth
    ipv6selfdns: True
    monrun2: True
    cauth_use: False
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/mdbhealth_getter.py
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: mdb_health_compute_prod
        service: yandexcloud_dbaas
    pg_ssl_balancer: mdb-health.private-api.cloud.yandex.net
    common:
        nginx:
            worker_connections: 8192
    walg:
        enabled: False

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - compute.prod.health
    - compute.prod.jaeger_agent
    - compute.prod.solomon

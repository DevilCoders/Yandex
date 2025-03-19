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
        - components.dbaas-compute-controlplane
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    dbaas:
        shard_hosts:
            - health-dbaas-preprod01f.cloud-preprod.yandex.net
            - health-dbaas-preprod01h.cloud-preprod.yandex.net
            - health-dbaas-preprod01k.cloud-preprod.yandex.net
        cluster_name: mdb-health-compute-preprod
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
        cluster: mdb_health_compute_preprod
    network:
        l3_slb:
            virtual_ipv6:
                - 2a0d:d6c0:0:ff1a::3bd
    pg_ssl_balancer: mdb-health.private-api.cloud-preprod.yandex.net
    common:
        nginx:
            worker_connections: 8192
    mdb_health:
        monrun:
            url: https://mdb-health.private-api.cloud-preprod.yandex.net
            verify: /opt/yandex/allCAs.pem
        disable-health: True
    walg:
        enabled: False

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - compute.preprod.health
    - compute.preprod.jaeger_agent

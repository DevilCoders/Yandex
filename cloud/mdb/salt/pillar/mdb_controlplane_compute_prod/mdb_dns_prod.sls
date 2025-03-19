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
        - components.nginx
        - components.supervisor
        - components.mdb-dns
        - components.jaeger-agent
        - components.yasmagent
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.monrun2.mdb-dns
        - components.dbaas-compute-controlplane
    dbaas:
        shard_hosts:
            - mdb-dns01f.yandexcloud.net
            - mdb-dns01h.yandexcloud.net
            - mdb-dns01k.yandexcloud.net
        cluster_name: mdb-dns-compute-prod
        vtype: compute
    yasmagent:
        instances:
            - mdbdns
    ipv6selfdns: True
    monrun2: True
    cauth_use: False
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: mdb_dns_compute_prod
        service: yandexcloud_dbaas
    pg_ssl_balancer: mdb-dns.private-api.cloud.yandex.net

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - compute.prod.dns
    - compute.prod.solomon
    - compute.prod.jaeger_agent

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
        - components.monrun2.mdb-dns
        - components.dbaas-compute-controlplane
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    dbaas:
        shard_hosts:
            - mdb-dns-dbaas-preprod01f.cloud-preprod.yandex.net
            - mdb-dns-dbaas-preprod01h.cloud-preprod.yandex.net
            - mdb-dns-dbaas-preprod01k.cloud-preprod.yandex.net
        cluster_name: mdb-dns-compute-preprod
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
        cluster: mdb_dns_compute_preprod
    pg_ssl_balancer: mdb-dns.private-api.cloud-preprod.yandex.net

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - compute.preprod.dns
    - compute.preprod.jaeger_agent

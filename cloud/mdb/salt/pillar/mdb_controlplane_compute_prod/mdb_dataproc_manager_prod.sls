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
        - components.envoy
        - components.redis
        - components.supervisor
        - components.mdb-dataproc-manager
        - components.monrun2.http-tls
        - components.dbaas-compute-controlplane
    ipv6selfdns: True
    monrun2: True
    cauth_use: False
    use_yasmagent: False
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        cluster: mdb_dataproc_manager_compute_prod
    cert:
        server_name: dataproc-manager.private-api.yandexcloud.net
    pg_ssl_balancer: dataproc-manager.private-api.yandexcloud.net
    iam:
        url: as.private-api.cloud.yandex.net:4286
    dbaas:
        vtype: compute
        shard_hosts:
            - dataproc-manager01f.yandexcloud.net
            - dataproc-manager01h.yandexcloud.net
            - dataproc-manager01k.yandexcloud.net
        cluster_name: mdb-dataproc-manager-redis-prod
    redis:
        config:
            loglevel: notice
    walg:
        enabled: False

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.solomon
    - compute.prod.dataproc_manager

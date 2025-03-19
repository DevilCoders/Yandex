mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.envoy
        - components.supervisor
        - components.redis
        - components.mdb-dataproc-manager
        - components.monrun2.http-tls
        - components.dbaas-compute-controlplane
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    ipv6selfdns: True
    monrun2: True
    cauth_use: False
    use_yasmagent: False
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        cluster: mdb_dataproc_manager_compute_preprod
    cert:
        server_name: dataproc-manager.private-api.cloud-preprod.yandex.net
    pg_ssl_balancer: dataproc-manager.private-api.cloud-preprod.yandex.net
    iam:
        url: as.private-api.cloud-preprod.yandex.net:4286
    dbaas:
        shard_hosts:
            - dataproc-manager-preprod01f.cloud-preprod.yandex.net
            - dataproc-manager-preprod01h.cloud-preprod.yandex.net
            - dataproc-manager-preprod01k.cloud-preprod.yandex.net
        cluster_name: mdb-dataproc-manager-redis-preprod
        vtype: compute
    redis:
        config:
            loglevel: notice
    walg:
        enabled: False

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - compute.preprod.dataproc_manager

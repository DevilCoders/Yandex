mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.nginx
        - components.deploy.mdb-deploy-api
        - components.jaeger-agent
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.dbaas-compute-controlplane
        - components.yasmagent
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    tvmtool:
        port: 50001
        tvm_id: 2011016
    mdb-deploy-api:
        extra-login-white-list:
            - robot-mdb-katan-ct
            - amatol
    ipv6selfdns: True
    monrun2: True
    use_yasmagent: True
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        cluster: mdb_deploy_api_compute_preprod
    cauth_use: False
    dbaas:
        vtype: compute
    network:
        l3_slb:
            virtual_ipv6:
                - 2a0d:d6c0:0:ff1a::20a
    mdb-deploy-cleaner:
        zk_hosts:
            - zk-dbaas-preprod01f.cloud-preprod.yandex.net:2181
            - zk-dbaas-preprod01h.cloud-preprod.yandex.net:2181
            - zk-dbaas-preprod01k.cloud-preprod.yandex.net:2181
    pg_ssl_balancer: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    disable_deploy_api: True

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - compute.preprod.deploy_api
    - compute.preprod.jaeger_agent
    - mdb_developers

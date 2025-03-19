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
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    iam:
        mdb_cloud_id: b1ggh9onj7ljr7m9cici
    tvmtool:
        port: 50001
        tvm_id: 2015827
    mdb-deploy-api:
        extra-login-white-list:
            - robot-mdb-katan-cp
            - amatol
    ipv6selfdns: True
    monrun2: True
    use_yasmagent: True
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: mdb_deploy_api_compute_prod
        service: yandexcloud_dbaas
    cauth_use: False
    dbaas:
        vtype: compute
    network:
        l3_slb:
            virtual_ipv6:
                - 2a0d:d6c0:0:1b::3b5
    pg_ssl_balancer: mdb-deploy-api.private-api.yandexcloud.net
    mdb-deploy-cleaner:
        zk_hosts:
            - zk-dbaas01f.yandexcloud.net:2181
            - zk-dbaas01h.yandexcloud.net:2181
            - zk-dbaas01k.yandexcloud.net:2181

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - compute.prod.mdb_deploy_api
    - compute.prod.jaeger_agent
    - compute.prod.solomon
    - mdb_developers

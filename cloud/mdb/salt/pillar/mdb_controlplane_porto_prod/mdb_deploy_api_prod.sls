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
        - components.yasmagent
        - components.dbaas-porto-controlplane
    monrun2: True
    use_yasmagent: True
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_deploy_api_porto_prod
        service: deploy_api
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    iam:
        mdb_cloud_id: fooe9dt3lm4bvrup1las
    tvmtool:
        port: 50001
        tvm_id: 2010506
    mdb-deploy-api:
        extra-login-white-list:
            - robot-mdb-katan-pp
            - robot-mdb-cms-porto
            - amatol
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:4
    pg_ssl_balancer: deploy-api.db.yandex-team.ru
    mdb-deploy-cleaner:
        zk_hosts:
            - zkeeper01e.db.yandex.net:2181
            - zkeeper01f.db.yandex.net:2181
            - zkeeper01h.db.yandex.net:2181

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - porto.prod.dbaas.mdb_deploy_api
    - porto.prod.dbaas.jaeger_agent
    - porto.prod.dbaas.solomon
    - mdb_developers

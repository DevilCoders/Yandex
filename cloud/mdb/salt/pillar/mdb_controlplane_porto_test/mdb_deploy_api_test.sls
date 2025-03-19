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
        cluster: mdb_deploy_api_porto_test
        service: deploy_api
    deploy:
        version: 2
        api_host: deploy-api-test.db.yandex-team.ru
    iam:
        mdb_cloud_id: fooe9dt3lm4bvrup1las
    tvmtool:
        port: 50001
        tvm_id: 2010842
    mdb-deploy-api:
        extra-login-white-list:
            - robot-mdb-katan-pt
            - robot-mdb-cms-porto
            - amatol
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:2
    pg_ssl_balancer: deploy-api-test.db.yandex-team.ru
    mdb-deploy-cleaner:
        zk_hosts:
            - zk-df-e2e01f.db.yandex.net:2181
            - zk-df-e2e01h.db.yandex.net:2181
            - zk-df-e2e01k.db.yandex.net:2181

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.selfdns
    - porto.test.dbaas.mdb_deploy_api
    - porto.test.dbaas.jaeger_agent
    - porto.test.dbaas.solomon
    - mdb_developers

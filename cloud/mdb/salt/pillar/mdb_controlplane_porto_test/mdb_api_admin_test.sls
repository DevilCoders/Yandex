mine_functions:
    grains.item:
        - id
        - role
        - ya
        - virtual

data:
    runlist:
        - components.web-api-base-lite
        - components.nginx
        - components.supervisor
        - components.dbaas-internal-api
        - components.jaeger-agent
        - components.dbaas-porto-controlplane
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    monrun2: True
    use_pushclient: True
    yasmagent:
        instances:
            - mdbapi
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_api_admin_porto_test
        service: mdb
    monrun:
        http_ping:
            warn: 405

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - porto.prod.selfdns.realm-mdb
    - porto.test.dbaas.internal_api
    - porto.test.dbaas.internal_api_admin
    - porto.prod.clickhouse.dbaas
    - porto.prod.pgusers.dev.dbaas_api
    - porto.prod.dbaas.solomon
    - generated.porto.internal_api_clickhouse_versions
    - porto.test.dbaas.jaeger_agent

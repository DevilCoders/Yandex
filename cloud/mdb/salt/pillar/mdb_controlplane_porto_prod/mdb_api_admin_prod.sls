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
    monrun:
        http_ping:
            warn: 405

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - porto.prod.dbaas.worker
    - porto.prod.dbaas.internal_api
    - porto.prod.dbaas.internal_api_admin
    - porto.prod.clickhouse.dbaas
    - porto.prod.pgusers.dbaas_api
    - porto.prod.dbaas.jaeger_agent
    - generated.porto.internal_api_clickhouse_versions

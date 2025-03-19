mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    runlist:
        - components.web-api-base-lite
        - components.nginx
        - components.supervisor
        - components.dbaas-internal-api
        - components.jaeger-agent
        - components.dbaas-compute-controlplane
    l3host: True
    ipv6selfdns: True
    monrun2: True
    use_pushclient: True
    dbaas:
        vtype: compute
    yasmagent:
        instances:
            - mdbapi
    cauth_use: False
    monrun:
        http_ping:
            warn: 405

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - compute.prod.worker
    - compute.prod.internal_api
    - compute.prod.internal_api_admin
    - compute.prod.clickhouse
    - compute.prod.pgusers.dbaas_api
    - compute.prod.jaeger_agent
    - generated.compute.internal_api_clickhouse_versions

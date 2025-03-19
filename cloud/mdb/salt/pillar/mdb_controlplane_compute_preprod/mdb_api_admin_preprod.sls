mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    runlist:
        - components.web-api-base-lite
        - components.nginx
        - components.supervisor
        - components.dbaas-internal-api
        - components.jaeger-agent
        - components.network
        - components.dbaas-compute-controlplane
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
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
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - compute.preprod.worker
    - compute.preprod.internal_api
    - compute.preprod.internal_api_admin
    - compute.preprod.clickhouse
    - compute.preprod.pgusers.dbaas_api
    - compute.preprod.jaeger_agent
    - generated.compute.internal_api_clickhouse_versions

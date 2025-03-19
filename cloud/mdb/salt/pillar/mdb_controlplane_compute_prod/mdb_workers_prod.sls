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
    condgroup: mdb_dbaas_workers
    runlist:
        - components.web-api-base-lite
        - components.supervisor
        - components.dbaas-worker
        - components.jaeger-agent
        - components.dbaas-compute-controlplane
    ipv6selfdns: True
    monrun2: True
    cauth_use: False
    dbaas:
        vtype: compute
    dbaas_worker:
        config:
            deploy:
                version: 2
                url_v2: https://mdb-deploy-api.private-api.yandexcloud.net
                group: compute-prod
                attempts: 4

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.ui
    - compute.prod.worker
    - compute.prod.pgusers.dbaas_api
    - compute.prod.pgusers.dbaas_worker
    - compute.prod.s3.pgaas_s3backup
    - compute.prod.jaeger_agent

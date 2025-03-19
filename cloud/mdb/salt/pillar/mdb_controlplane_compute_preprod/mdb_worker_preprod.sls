mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    condgroup: mdb_dbaas_workers
    runlist:
        - components.web-api-base-lite
        - components.supervisor
        - components.dbaas-worker
        - components.jaeger-agent
        - components.dbaas-compute-controlplane
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    ipv6selfdns: True
    monrun2: True
    monrun:
        orphan_resources_args: ' --exclude-label=purpose:dataproc_infratest -e ''(.*(dataproc|infratest).*)'''
    cauth_use: False
    dbaas:
        vtype: compute
    dbaas_worker:
        config:
            deploy:
                version: 2
                url_v2: https://mdb-deploy-api.private-api.cloud-preprod.yandex.net
                group: compute-preprod
                attempts: 4
    k8s_workers:
        - worker-0.worker.preprod.svc.cluster.local
        - worker-1.worker.preprod.svc.cluster.local
        - worker-2.worker.preprod.svc.cluster.local
    disable_worker: True

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.ui
    - compute.preprod.worker
    - compute.preprod.pgusers.dbaas_api
    - compute.preprod.pgusers.dbaas_worker
    - compute.preprod.s3.pgaas_s3backup
    - compute.preprod.jaeger_agent

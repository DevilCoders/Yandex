mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.dataproc-ui-proxy
        - components.dbaas-compute-controlplane
    l3host: True
    ipv6selfdns: True
    monrun2: True
    use_yasmagent: True
    use_pushclient: True
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    dbaas:
        vtype: compute
    cauth_use: False

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - compute.prod.dataproc_ui_proxy

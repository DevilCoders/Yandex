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
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    dbaas:
        vtype: compute
    cauth_use: False
include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - compute.preprod.dataproc_ui_proxy

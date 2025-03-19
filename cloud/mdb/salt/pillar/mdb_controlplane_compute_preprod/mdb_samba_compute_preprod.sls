mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.yasmagent
        - components.dbaas-compute-controlplane

    use_yasmagent: False
    ipv6selfdns: True
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon

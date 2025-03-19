data:
    runlist:
        - components.deploy.salt-master
        - components.redis
        - components.dbaas-compute-controlplane
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.mdb-salt-sync
        - components.mdb-controlplane-telegraf
    solomon:
        cluster: mdb_deploy_salt_compute_preprod
    salt_master:
        use_prometheus_metrics: true
    telegraf:
        prometheus:
            interval: '1s'

include:
    - mdb_controlplane_compute_preprod.deploy_salt.common

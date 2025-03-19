data:
    runlist:
        - components.deploy.salt-master
        - components.redis
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.mdb-salt-sync
        - components.dbaas-porto-controlplane
        - components.mdb-controlplane-telegraf
    solomon:
        cluster: mdb_deploy_salt_porto_test
    salt_master:
        use_prometheus_metrics: true
    telegraf:
        prometheus:
            interval: '1s'

include:
    - mdb_controlplane_porto_test.deploy_salt.common

data:
    runlist:
        - components.deploy.salt-master
        - components.redis
        - components.dbaas-compute-controlplane
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.mdb-salt-sync
    mdb_salt_sync:
        personal: True
    salt_master:
        use_s3_images: False
    solomon:
        cluster: mdb_deploy_salt_compute_preprod_personal

include:
    - mdb_controlplane_compute_preprod.deploy_salt.common

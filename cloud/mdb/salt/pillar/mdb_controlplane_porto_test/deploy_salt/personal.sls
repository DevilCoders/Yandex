data:
    runlist:
        - components.deploy.salt-master
        - components.redis
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.mdb-salt-sync
        - components.dbaas-porto-controlplane
    salt_master:
        use_s3_images: False
    mdb_salt_sync:
        personal: True
    solomon:
        cluster: mdb_deploy_salt_porto_test_personal

include:
    - mdb_controlplane_porto_test.deploy_salt.common

data:
    mdb_salt_sync:
        personal: True
    salt_master:
        use_s3_images: False

include:
    - mdb_controlplane_porto_prod.common
    - mdb_controlplane_porto_prod.mdb_deploy_salt_prod

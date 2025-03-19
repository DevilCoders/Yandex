include:
    - compute.preprod.environment
    - compute.preprod.selfdns.realm-cloud-mdb
    - mdb_controlplane_compute_preprod.common.access_service
    - mdb_controlplane_compute_preprod.common.token_service
    - mdb_controlplane_compute_preprod.common.tls_monrun
    - mdb_controlplane_compute_preprod.common.kernel

data:
    ipv6selfdns: True
    iam:
        mdb_cloud_id: aoe9shbqc2v314v7fp3d
    dist:
        bionic:
            secure: True
        pgdg:
            absent: True
    sentry:
        environment: compute-preprod
    system:
        journald:
            disable_ratelimit: True

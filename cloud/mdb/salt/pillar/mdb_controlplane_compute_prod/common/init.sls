include:
    - compute.prod.environment
    - compute.prod.selfdns.realm-cloud-mdb
    - mdb_controlplane_compute_prod.common.access_service
    - mdb_controlplane_compute_prod.common.kernel
    - mdb_controlplane_compute_prod.common.token_service
    - mdb_controlplane_compute_prod.common.tls_monrun

data:
    ipv6selfdns: True
    dist:
        bionic:
            secure: True
        pgdg:
            absent: True
    sentry:
        environment: compute-prod

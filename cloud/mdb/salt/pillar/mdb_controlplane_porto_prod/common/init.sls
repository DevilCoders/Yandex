include:
    - porto.prod.environment
    - porto.prod.selfdns.realm-mdb
    - mdb_controlplane_porto_prod.common.access_service
    - mdb_controlplane_porto_prod.common.tls_monrun

data:
    ipv6selfdns: True
    # used in components https://nda.ya.ru/t/6Xzic3r53bghBT
    dbaas:
        vtype: porto
    dist:
        bionic:
            secure: True
        pgdg:
            absent: True
    sentry:
        environment: porto-prod
    cauth_use_v2: True


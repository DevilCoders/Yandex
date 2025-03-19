include:
    - porto.test.environment
    - mdb_controlplane_porto_test.common.access_service
    - mdb_controlplane_porto_test.common.tls_monrun
    - mdb_controlplane_porto_test.common.kernel

data:
    # used in components https://nda.ya.ru/t/6Xzic3r53bghBT
    dbaas:
        vtype: porto
    dist:
        bionic:
            secure: True
        pgdg:
            absent: True
    cauth_use_v2: True
    sentry:
        environment: porto-test
    system:
        journald:
            disable_ratelimit: True

data:
    dbaas:
        vtype: compute
    runlist:
        - components.web-api-base
        - components.mdb-ui
        - components.jaeger-agent
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.dbaas-compute-controlplane
    pg_ssl_balancer: {{ salt.grains.get('id') }}
    pg_ssl_balancer_alt_names:
        - ui-compute-prod.db.yandex-team.ru
    monrun2: True
    ipv6selfdns: True
    sysctl:
        vm.nr_hugepages: 0
    common:
        dh: |
            {{ salt.yav.get('ver-01evy2y64h1g0xsqd20z0nqv1r[secret]') | indent(12) }}
    mdbui:
        installation: compute-prod
        base_host: ui-compute-prod.db.yandex-team.ru
        deploydb:
            host: 'mdb-deploy-db01f.yandexcloud.net,mdb-deploy-db01h.yandexcloud.net,mdb-deploy-db01k.yandexcloud.net'
            password: {{ salt.yav.get('ver-01eg07gmcbtcg4bs75a2q51sh3[password]') }}
        metadb:
            host: 'meta-dbaas01f.yandexcloud.net,meta-dbaas01h.yandexcloud.net,meta-dbaas01k.yandexcloud.net'
            password: {{ salt.yav.get('ver-01eg07gmcbtcg4bs75a2q51sh3[password]') }}
        katandb:
            host: 'mdb-katandb01-rc1a.yandexcloud.net,mdb-katandb01-rc1b.yandexcloud.net,mdb-katandb01-rc1c.yandexcloud.net'
            password: {{ salt.yav.get('ver-01eg07gmcbtcg4bs75a2q51sh3[password]') }}
        cmsdb:
            host: 'mdb-cmsdb01-rc1a.yandexcloud.net,mdb-cmsdb01-rc1b.yandexcloud.net,mdb-cmsdb01-rc1c.yandexcloud.net'
            password: {{ salt.yav.get('ver-01eg07gmcbtcg4bs75a2q51sh3[password]') }}
        mlockdb:
            host: 'mlockdb01f.yandexcloud.net,mlockdb01h.yandexcloud.net,mlockdb01k.yandexcloud.net'
            password: {{ salt.yav.get('ver-01eg07gmcbtcg4bs75a2q51sh3[password]') }}
        tvm_client_id: 2025704
        tvm_secret: {{ salt.yav.get('ver-01ew2wk8rk0j1w6yyxgkt0d1cd[secret]') }}
        secret_key: {{ salt.yav.get('ver-01evy2fxn7wtew840e7hf02rp2[secret]') }}
        sentry_dsn: {{ salt.yav.get('ver-01ezac375ya8gpyvf8s0fz1xpv[secret]') }}

include:
    - common
    - mdb_developers
    - mdb_supports
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.solomon
    - compute.prod.jaeger_agent

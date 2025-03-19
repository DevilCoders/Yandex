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
        - ui-preprod.db.yandex-team.ru
    monrun2: True
    ipv6selfdns: True
    sysctl:
        vm.nr_hugepages: 0
    common:
        dh: |
            {{ salt.yav.get('ver-01evy2zmfkpj2651kff4z2bm9b[secret]') | indent(12) }}
    mdbui:
        installation: compute-preprod
        base_host: ui-preprod.db.yandex-team.ru
        deploydb:
            host: 'mdb-deploy-db-preprod01f.cloud-preprod.yandex.net,mdb-deploy-db-preprod01h.cloud-preprod.yandex.net,mdb-deploy-db-preprod01k.cloud-preprod.yandex.net'
            password: {{ salt.yav.get('ver-01eg07bbgg9vj5ye7dn1dg79dn[password]') }}
        metadb:
            host: 'meta-dbaas-preprod01f.cloud-preprod.yandex.net,meta-dbaas-preprod01h.cloud-preprod.yandex.net,meta-dbaas-preprod01k.cloud-preprod.yandex.net'
            password: {{ salt.yav.get('ver-01eg07bbgg9vj5ye7dn1dg79dn[password]') }}
        katandb:
            host: 'katan-db-preprod01f.cloud-preprod.yandex.net,katan-db-preprod01h.cloud-preprod.yandex.net,katan-db-preprod01k.cloud-preprod.yandex.net'
            password: {{ salt.yav.get('ver-01eg07bbgg9vj5ye7dn1dg79dn[password]') }}
        cmsdb:
            host: 'mdb-cmsdb-preprod01-rc1a.cloud-preprod.yandex.net,mdb-cmsdb-preprod01-rc1c.cloud-preprod.yandex.net,mdb-cmsdb-preprod01-rc1b.cloud-preprod.yandex.net'
            password: {{ salt.yav.get('ver-01eg07bbgg9vj5ye7dn1dg79dn[password]') }}
        mlockdb:
            host: 'mlockdb-preprod01f.cloud-preprod.yandex.net,mlockdb-preprod01h.cloud-preprod.yandex.net,mlockdb-preprod01k.cloud-preprod.yandex.net'
            password: {{ salt.yav.get('ver-01eg07bbgg9vj5ye7dn1dg79dn[password]') }}
        tvm_client_id: 2025702
        tvm_secret: {{ salt.yav.get('ver-01ew2wgr8a05jy4zsbnstmk7ba[secret]') }}
        secret_key: {{ salt.yav.get('ver-01evy2hgq3x3qbttvt8t5xg1tg[secret]') }}
        sentry_dsn: {{ salt.yav.get('ver-01ezac375ya8gpyvf8s0fz1xpv[secret]') }}
        white_list:
           - mysql_cluster_modify
           - postgresql_cluster_modify

include:
    - common
    - mdb_developers
    - mdb_supports
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - compute.preprod.jaeger_agent

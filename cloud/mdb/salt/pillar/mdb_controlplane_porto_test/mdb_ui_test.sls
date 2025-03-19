data:
    runlist:
        - components.web-api-base
        - components.mdb-ui
        - components.jaeger-agent
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.network
    pg_ssl_balancer: {{ salt.grains.get('id') }}
    pg_ssl_balancer_alt_names:
        - ui.db.yandex-team.ru
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:f
    monrun2: True
    sysctl:
        vm.nr_hugepages: 0
    common:
        dh: |
            {{ salt.yav.get('ver-01entfaxpz5965xnxnme49fatv[secret]') | indent(12) }}
    mdbui:
        installation: porto-test
        base_host: ui.db.yandex-team.ru
        deploydb:
            host: 'deploy-db-test01f.db.yandex.net,deploy-db-test01h.db.yandex.net,deploy-db-test01k.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg0721msapnd0s78ps9q4dw3[password]') }}
        metadb:
            host: 'meta-test01f.db.yandex.net,meta-test01k.db.yandex.net,meta-test01h.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg0721msapnd0s78ps9q4dw3[password]') }}
        katandb:
            host: 'katan-db-test01h.db.yandex.net,katan-db-test01f.db.yandex.net,katan-db-test01k.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg0721msapnd0s78ps9q4dw3[password]') }}
        dbmdb:
            host: 'dbm-test01h.db.yandex.net,dbm-test01f.db.yandex.net,dbm-test01k.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg0721msapnd0s78ps9q4dw3[password]') }}
        cmsdb:
            host: 'cms-db-test01f.db.yandex.net,cms-db-test01h.db.yandex.net,cms-db-test01k.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg0721msapnd0s78ps9q4dw3[password]') }}
        mlockdb:
            host: 'mlockdb-test01f.db.yandex.net,mlockdb-test01h.db.yandex.net,mlockdb-test01k.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg0721msapnd0s78ps9q4dw3[password]') }}
        tvm_client_id: 2024487
        tvm_secret: {{ salt.yav.get('ver-01ep43agdv263y2dc9ywpc0y2g[secret]') }}
        secret_key: {{ salt.yav.get('ver-01eqbb7qkakr6c3bkha3md4cfh[secret]') }}
        sentry_dsn: {{ salt.yav.get('ver-01ezac375ya8gpyvf8s0fz1xpv[secret]') }}
        white_list:
           - mysql_cluster_modify
           - postgresql_cluster_modify

include:
    - common
    - mdb_developers
    - envs.dev
    - porto.test.selfdns.realm-mdb
    - porto.test.dbaas.jaeger_agent

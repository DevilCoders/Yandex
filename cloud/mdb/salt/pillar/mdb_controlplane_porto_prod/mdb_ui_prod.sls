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
        - ui-prod.db.yandex-team.ru
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:10
    monrun2: True
    sysctl:
        vm.nr_hugepages: 0
    common:
        dh: |
            {{ salt.yav.get('ver-01etd7a2e8a3ebq7pkjz9jjqbd[secret]') | indent(12) }}
    mdbui:
        installation: porto-prod
        base_host: ui-prod.db.yandex-team.ru
        deploydb:
            host: 'deploy-db01f.db.yandex.net,deploy-db01h.db.yandex.net,deploy-db01k.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg075spjrkvtzng7zzhhf3dm[password]') }}
        metadb:
            host: 'meta01k.db.yandex.net,meta01h.db.yandex.net,meta01f.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg075spjrkvtzng7zzhhf3dm[password]') }}
        katandb:
            host: 'katan-db01f.db.yandex.net,katan-db01h.db.yandex.net,katan-db01k.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg075spjrkvtzng7zzhhf3dm[password]') }}
        dbmdb:
            host: 'dbmdb01f.db.yandex.net,dbmdb01h.db.yandex.net,dbmdb01k.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg075spjrkvtzng7zzhhf3dm[password]') }}
        cmsdb:
            host: 'cms-db-prod01f.db.yandex.net,cms-db-prod01h.db.yandex.net,cms-db-prod01k.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg075spjrkvtzng7zzhhf3dm[password]') }}
        mlockdb:
            host: 'mlockdb01f.db.yandex.net,mlockdb01h.db.yandex.net,mlockdb01k.db.yandex.net'
            password: {{ salt.yav.get('ver-01eg075spjrkvtzng7zzhhf3dm[password]') }}
        tvm_client_id: 2025558
        tvm_secret: {{ salt.yav.get('ver-01etd6hcz5bxpm9a502nfme42t[secret]') }}
        secret_key: {{ salt.yav.get('ver-01etd721d9pq69tgrn1g8fqxwa[secret]') }}
        sentry_dsn: {{ salt.yav.get('ver-01ezac375ya8gpyvf8s0fz1xpv[secret]') }}
        white_list:
           - mysql_cluster_modify
           - postgresql_cluster_modify

include:
    - common
    - mdb_developers
    - envs.prod
    - porto.prod.selfdns.realm-mdb
    - porto.prod.dbaas.jaeger_agent

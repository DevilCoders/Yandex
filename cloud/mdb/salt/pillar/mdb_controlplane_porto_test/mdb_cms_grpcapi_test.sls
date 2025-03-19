mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.envoy
        - components.mdb-cms-grpcapi
        - components.jaeger-agent
        - components.monrun2.grpc-ping
        - components.monrun2.http-tls
        - components.dbaas-porto-controlplane
        - components.yasmagent
    pg_ssl_balancer: mdb-cmsgrpcapi-test.db.yandex.net
    cert:
        server_name: mdb-cmsgrpcapi-test.db.yandex.net
    envoy:
        use_health_map: true
        clusters:
            mdb-cms:
                prefix: "/"
                port: 30030
    use_yasmagent: False
    solomon:
        cluster: mdb_cms_grpcapi_porto_test
    slb_close_file: /tmp/.mdb-cms-close
    cmsdb:
        addrs:
            - cms-db-test01f.db.yandex.net:6432
            - cms-db-test01h.db.yandex.net:6432
            - cms-db-test01k.db.yandex.net:6432
    metadb:
        addrs:
            - meta-test01f.db.yandex.net:6432
            - meta-test01h.db.yandex.net:6432
            - meta-test01k.db.yandex.net:6432
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:11
    cms:
        auth:
            skip_auth_errors: false
            folder_id: fooi5vu9rdejqc3p4b60
    dbm:
        token: {{ salt.yav.get('ver-01e8m5zx0e9vjpy59wgq3n2699[dbm_token]') }}
        host: mdb-test.db.yandex-team.ru
    conductor:
        oauth: {{ salt.yav.get('ver-01fsrzc3ngp9p8gr3r7t2fs7ys[conductor]') }}

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.selfdns
    - mdb_controlplane_porto_test.common.solomon
    - mdb_controlplane_porto_test.common.deploy
    - mdb_controlplane_porto_test.common.groups_lists
    - mdb_controlplane_porto_test.common.ui
    - porto.test.pgusers.cms
    - mdb_controlplane_porto_test.common.mdb_metrics
    - porto.test.dbaas.jaeger_agent

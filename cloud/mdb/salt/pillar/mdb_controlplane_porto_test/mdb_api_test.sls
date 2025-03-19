mine_functions:
    grains.item:
        - id
        - role
        - ya
        - virtual

data:
    runlist:
        - components.web-api-base-lite
        - components.nginx
        - components.supervisor
        - components.dbaas-internal-api
        - components.jaeger-agent
        - components.network
        - components.dbaas-porto-controlplane
        - components.monrun2.http-tls
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    l3host: True
    use_pushclient: True
    pg_ssl_balancer: internal-api-test.db.yandex-team.ru
    cert:
        server_name: internal-api-test.db.yandex-team.ru
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_api_porto_test
        service: mdb
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:e
    monrun2: True
    monrun:
        http_ping:
            warn: 405
    search-reindexer:
        sentry:
            dsn: {{ salt.yav.get('ver-01f33e2vk953vsn1egx3m1h2kd[dsn]') }}
        service_account:
            id: yc.mdb.search-reindexer
            key_id: {{ salt.yav.get('ver-01f33gnmpk1ze5y98s0r7je3s7[key_id]') }}
            private_key: {{ salt.yav.get('ver-01f33gnmpk1ze5y98s0r7je3s7[private_key]') | yaml_encode }}

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.metadb
    - mdb_controlplane_porto_test.common.token_service
    - mdb_controlplane_porto_test.common.zk
    - porto.prod.selfdns.realm-mdb
    - porto.test.dbaas.internal_api
    - porto.prod.clickhouse.dbaas
    - porto.prod.pgusers.dev.dbaas_api
    - porto.prod.dbaas.solomon
    - generated.porto.internal_api_clickhouse_versions
    - porto.test.dbaas.jaeger_agent

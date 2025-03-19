mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
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
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_internal_api_porto_prod
        service: mdb
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:55d:0:3
    pg_ssl_balancer: internal-api.db.yandex-team.ru
    pg_ssl_balancer_alt_names:
        - internal-api.db.yandex.net
        - api.db.yandex-team.ru
    monrun2: True
    monrun:
        http_ping:
            warn: 405
    search-reindexer:
        sentry:
            dsn: {{ salt.yav.get('ver-01f3q3vtdedpr6v9j8jm97wh9b[dsn]') }}
        service_account:
            id: yc.mdb.search-reindexer
            key_id: {{ salt.yav.get('ver-01f33gnmpk1ze5y98s0r7je3s7[key_id]') }}
            private_key: {{ salt.yav.get('ver-01f33gnmpk1ze5y98s0r7je3s7[private_key]') | yaml_encode }}

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - mdb_controlplane_porto_prod.common.metadb
    - mdb_controlplane_porto_prod.common.token_service
    - mdb_controlplane_porto_prod.common.zk
    - porto.prod.dbaas.internal_api
    - porto.prod.clickhouse.dbaas
    - porto.prod.pgusers.dbaas_api
    - porto.prod.dbaas.solomon
    - porto.prod.dbaas.jaeger_agent
    - generated.porto.internal_api_clickhouse_versions

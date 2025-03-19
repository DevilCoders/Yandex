mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    condgroup: mdb_internal_api
    internal_api:
        env: prod
    runlist:
        - components.web-api-base-lite
        - components.nginx
        - components.supervisor
        - components.dbaas-internal-api
        - components.jaeger-agent
        - components.dbaas-compute-controlplane
        - components.monrun2.http-tls
    l3host: True
    ipv6selfdns: True
    monrun2: True
    use_pushclient: True
    dbaas:
        vtype: compute
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: mdb_internal_api_compute_prod
        service: yandexcloud_dbaas
    yasmagent:
        instances:
            - mdbapi
    cauth_use: False
    monrun:
        http_ping:
            warn: 405
    pg_ssl_balancer: mdb.private-api.cloud.yandex.net
    search-reindexer:
        sentry:
            dsn: {{ salt.yav.get('ver-01f3q4fjr102hh1cebs5dmyh4c[dsn]') }}
        service_account:
            id: yc.mdb.search-reindexer
            key_id: {{ salt.yav.get('ver-01f3qmw77a5r2vzyqcsqraq2p1[key_id]') }}
            private_key: {{ salt.yav.get('ver-01f3qmw77a5r2vzyqcsqraq2p1[private_key]') | yaml_encode }}

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.metadb
    - mdb_controlplane_compute_prod.common.zk
    - compute.prod.internal_api
    - compute.prod.clickhouse
    - compute.prod.pgusers.dbaas_api
    - compute.prod.solomon
    - compute.prod.jaeger_agent
    - generated.compute.internal_api_clickhouse_versions

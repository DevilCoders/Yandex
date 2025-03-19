mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.envoy
        - components.mdb-internal-api
        - components.mdb-controlplane-telegraf
        - components.jaeger-agent
        - components.monrun2.grpc-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.dbaas-porto-controlplane
    monrun2: True
    use_yasmagent: False
    use_pushclient: True
    yasmagent:
        instances:
            - mdbinternalapi
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        cluster: mdb_internal_api_porto_prod
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    pg_ssl_balancer: mdb-internal-api.db.yandex.net
    cert:
        server_name: mdb-internal-api.db.yandex.net
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:7
    slb_close_file: /tmp/.mdb-internal-api-close
    mdb-search-reindexer:
        sentry:
            dsn: {{ salt.yav.get('ver-01fb2mrg2f8dxw9fc15a6zchyx[dsn]') }}

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - mdb_controlplane_porto_prod.common.solomon
    - mdb_controlplane_porto_prod.common.zk
    - porto.prod.dbaas.mdb_internal_api
    - porto.prod.dbaas.prefixes
    - porto.prod.dbaas.jaeger_agent
    - generated.porto.mdb_internal_api_clickhouse_versions
    - mdb_internal_api_db_versions.porto.elasticsearch

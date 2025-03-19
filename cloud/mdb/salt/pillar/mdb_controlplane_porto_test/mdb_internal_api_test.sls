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
        cluster: mdb_internal_api_porto_test
    deploy:
        version: 2
        api_host: deploy-api-test.db.yandex-team.ru
    pg_ssl_balancer: mdb-internal-api-test.db.yandex.net
    cert:
        server_name: mdb-internal-api-test.db.yandex.net
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:6
    slb_close_file: /tmp/.mdb-internal-api-close
    mdb-search-reindexer:
        sentry:
            dsn: {{ salt.yav.get('ver-01fb2mnqnw2wt96z0fpadyemmf[dsn]') }}

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.selfdns
    - mdb_controlplane_porto_test.common.solomon
    - mdb_controlplane_porto_test.common.zk
    - porto.test.dbaas.mdb_internal_api
    - porto.test.dbaas.prefixes
    - porto.test.dbaas.jaeger_agent
    - generated.porto.mdb_internal_api_clickhouse_versions
    - mdb_internal_api_db_versions.porto.elasticsearch

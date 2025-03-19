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
        - components.dbaas-compute-controlplane
        - components.yasmagent
    l3host: True
    ipv6selfdns: True
    monrun2: True
    use_yasmagent: True
    use_pushclient: True
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    yasmagent:
        instances:
            - mdbinternalapi
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        cluster: mdb_internal_api_prod
    pg_ssl_balancer: mdb-internal-api.private-api.cloud.yandex.net
    cert:
        server_name: mdb-internal-api.private-api.cloud.yandex.net
    dbaas:
        vtype: compute
    cauth_use: False
    slb_close_file: /tmp/.mdb-internal-api-close
    mdb-search-reindexer:
        sentry:
            dsn: {{ salt.yav.get('ver-01fb2mv6dg7xhb2jmv5jggq5sy[dsn]') }}
    system:
        journald:
            disable_ratelimit: True

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.solomon
    - mdb_controlplane_compute_prod.common.zk
    - compute.prod.mdb_internal_api
    - compute.prod.prefixes
    - compute.prod.jaeger_agent
    - generated.compute.mdb_internal_api_clickhouse_versions
    - mdb_internal_api_db_versions.compute.elasticsearch

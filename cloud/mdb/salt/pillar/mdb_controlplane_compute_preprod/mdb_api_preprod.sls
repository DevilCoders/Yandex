mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    condgroup: mdb_internal_api
    internal_api:
        env: preprod
    runlist:
        - components.web-api-base-lite
        - components.nginx
        - components.supervisor
        - components.dbaas-internal-api
        - components.jaeger-agent
        - components.network
        - components.dbaas-compute-controlplane
        - components.monrun2.http-tls
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    pg_ssl_balancer: mdb.private-api.cloud-preprod.yandex.net
    l3host: True
    ipv6selfdns: True
    monrun2: True
    use_pushclient: True
    dbaas:
        vtype: compute
    solomon:
        cluster: mdb_internal_api_compute_preprod
    yasmagent:
        instances:
            - mdbapi
    cauth_use: False
    monrun:
        http_ping:
            warn: 405
    search-reindexer:
        sentry:
            dsn: {{ salt.yav.get('ver-01f3dv96snx671gk6s6qmcswev[dsn]') }}
        service_account:
            id: yc.mdb.search-reindexer
            key_id: {{ salt.yav.get('ver-01f3dv4ynwc3ftma0efm838tzr[key_id]') }}
            private_key: {{ salt.yav.get('ver-01f3dv4ynwc3ftma0efm838tzr[private_key]') | yaml_encode }}

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - mdb_controlplane_compute_preprod.common.metadb
    - mdb_controlplane_compute_preprod.common.zk
    - compute.preprod.internal_api
    - compute.preprod.clickhouse
    - compute.preprod.pgusers.dbaas_api
    - compute.preprod.jaeger_agent
    - generated.compute.internal_api_clickhouse_versions

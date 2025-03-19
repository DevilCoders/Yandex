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
        - components.dbaas-compute-controlplane
        - components.yasmagent
    pg_ssl_balancer: mdb-cmsgrpcapi.private-api.cloud.yandex.net
    cert:
        server_name: mdb-cmsgrpcapi.private-api.cloud.yandex.net
    envoy:
        use_health_map: true
        clusters:
            mdb-cms:
                prefix: "/"
                port: 30030
    dbaas:
        vtype: compute
    use_yasmagent: False
    ipv6selfdns: True
    cauth_use: False
    solomon:
        cluster: mdb_cms_grpcapi_compute_prod
    slb_close_file: /tmp/.mdb-cms-close
    cmsdb:
        addrs:
            - mdb-cmsdb01-rc1a.yandexcloud.net:6432
            - mdb-cmsdb01-rc1b.yandexcloud.net:6432
            - mdb-cmsdb01-rc1c.yandexcloud.net:6432
    metadb:
        addrs:
            - meta-dbaas01f.yandexcloud.net:6432
            - meta-dbaas01h.yandexcloud.net:6432
            - meta-dbaas01k.yandexcloud.net:6432
    cms:
        auth:
            skip_auth_errors: false
            folder_id: b1g0r9fh49hee3rsc0aa
        is_compute: True
        fqdn_suffixes:
            controlplane: "yandexcloud.net"
            unmanaged_dataplane: "mdb.yandexcloud.net"
            managed_dataplane: "db.yandex.net"

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.solomon
    - mdb_controlplane_compute_prod.common.deploy
    - mdb_controlplane_compute_prod.common.ui
    - compute.prod.pgusers.cms
    - mdb_controlplane_compute_prod.common.mdb_metrics
    - mdb_controlplane_compute_prod.common.enabled_mw
    - compute.prod.jaeger_agent

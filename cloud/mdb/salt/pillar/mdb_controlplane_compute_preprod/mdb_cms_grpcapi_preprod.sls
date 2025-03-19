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
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    pg_ssl_balancer: mdb-cmsgrpcapi.private-api.cloud-preprod.yandex.net
    cert:
        server_name: mdb-cmsgrpcapi.private-api.cloud-preprod.yandex.net
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
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        cluster: mdb_cms_grpcapi_compute_preprod
    slb_close_file: /tmp/.mdb-cms-close
    cmsdb:
        addrs:
            - mdb-cmsdb-preprod01-rc1a.cloud-preprod.yandex.net:6432
            - mdb-cmsdb-preprod01-rc1c.cloud-preprod.yandex.net:6432
            - mdb-cmsdb-preprod01-rc1b.cloud-preprod.yandex.net:6432
    metadb:
        addrs:
            - meta-dbaas-preprod01f.cloud-preprod.yandex.net:6432
            - meta-dbaas-preprod01h.cloud-preprod.yandex.net:6432
            - meta-dbaas-preprod01k.cloud-preprod.yandex.net:6432
    cms:
        auth:
            skip_auth_errors: false
            folder_id: aoeme1ci0qvbsjia4ks7
        is_compute: True
        fqdn_suffixes:
            controlplane: "cloud-preprod.yandex.net"
            unmanaged_dataplane: "mdb.cloud-preprod.yandex.net"
            managed_dataplane: "db.yandex.net"

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - mdb_controlplane_compute_preprod.common.ui
    - compute.preprod.pgusers.cms
    - mdb_controlplane_compute_preprod.common.mdb_metrics
    - compute.preprod.jaeger_agent

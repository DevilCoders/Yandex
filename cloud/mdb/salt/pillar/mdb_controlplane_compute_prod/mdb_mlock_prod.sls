mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.envoy
        - components.mdb-mlock
        - components.monrun2.grpc-ping
        - components.monrun2.http-tls
        - components.dbaas-compute-controlplane
        - components.yasmagent
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    pg_ssl_balancer: mlock.private-api.yandexcloud.net
    cert:
        server_name: mlock.private-api.yandexcloud.net
    envoy:
        use_health_map: true
        clusters:
            mdb-mlock:
                prefix: "/"
                port: 30030
    mlock:
        mlockdb:
            addrs:
                - mlockdb01f.yandexcloud.net:6432
                - mlockdb01h.yandexcloud.net:6432
                - mlockdb01k.yandexcloud.net:6432
            db: mlockdb
            user: mlock
            sslmode: verify-full
            sslrootcert: /opt/yandex/allCAs.pem
            max_open_conn: 32
            max_idle_conn: 32
        use_auth: True
        auth:
            addr: as.private-api.cloud.yandex.net:4286
            permission: mdb.internal.mlock
            folder_id: b1g0r9fh49hee3rsc0aa
    dbaas:
        vtype: compute
    use_yasmagent: False
    ipv6selfdns: True
    cauth_use: False
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: 'mdb_{ctype}'
        service: yandexcloud_dbaas
    slb_close_file: /tmp/.mdb-mlock-close

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - compute.prod.pgusers.mlock
    - compute.prod.pgusers.mlockdb_admin
    - compute.prod.mdb_mlock
    - compute.prod.solomon

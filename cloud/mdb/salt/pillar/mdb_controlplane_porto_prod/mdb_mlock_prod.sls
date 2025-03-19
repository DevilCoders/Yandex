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
        - components.yasmagent
        - components.dbaas-porto-controlplane
    monrun2: True
    use_yasmagent: False
    use_pushclient: True
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: mdb_mlock_prod
        service: mdb
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    pg_ssl_balancer: mlock.db.yandex.net
    cert:
        server_name: mlock.db.yandex.net
    envoy:
        use_health_map: true
        clusters:
            mdb-mlock:
                prefix: "/"
                port: 30030
    mlock:
        mlockdb:
            addrs:
                - mlockdb01h.db.yandex.net:6432
                - mlockdb01f.db.yandex.net:6432
                - mlockdb01k.db.yandex.net:6432
            db: mlockdb
            user: mlock
            sslmode: verify-full
            sslrootcert: /opt/yandex/allCAs.pem
            max_open_conn: 32
            max_idle_conn: 32
        use_auth: True
        auth:
            addr: as.cloud.yandex-team.ru:4286
            permission: mdb.internal.mlock
            folder_id: fooi5vu9rdejqc3p4b60
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:d
    slb_close_file: /tmp/.mdb-mlock-close

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - porto.prod.pgusers.mlock
    - porto.prod.dbaas.mdb_mlock
    - porto.prod.dbaas.solomon

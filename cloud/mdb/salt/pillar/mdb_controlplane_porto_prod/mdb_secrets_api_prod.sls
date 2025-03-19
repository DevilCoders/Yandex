mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.nginx
        - components.jaeger-agent
        - components.mdb-secrets
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.tvmtool
        - components.dbaas-porto-controlplane
    tvmtool:
        port: 50001
        tvm_id: 2011396
        config:
            client: mdb-secrets
            secret: {{ salt.yav.get('ver-01e07k7696z28s5zkn0wp83qnj[secret]') }}
        token: {{ salt.yav.get('ver-01e07k7696z28s5zkn0wp83qnj[token]') }}
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:a64:0:3
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
        use_yasmagent: False
    solomon:
        cluster: secrets-api
    common:
        dh: |
            {{ salt.yav.get('ver-01e07kbk4f4ag13e6egw0f84h0[secret]') | indent(12) }}
    mdb-secrets:
        server_name:
            mdb-secrets.db.yandex.net
        config:
            app:
                sentry:
                    dsn: {{ salt.yav.get('ver-01f9kv20wppcvnddsre9ckxvbj[dsn]') }}
            saltkey: {{ salt.yav.get('ver-01e07mvgrq9j6vmqth2b6xzzjy[key]') }}
            privatekey: {{ salt.yav.get('ver-01e07mxjaq95fk3yeavhatknq4[key]') }}
            certificate_to_use: yandex
            yandex_crt:
                abc: 2660
                oauth: {{ salt.yav.get('ver-01e07pq7vs5x96swgtzrph7ywq[token]') }}
            secretsdb:
                addrs:
                 - secrets-db01h.db.yandex.net:6432
                 - secrets-db01f.db.yandex.net:6432
                 - secrets-db01k.db.yandex.net:6432
                user: secrets_api
                password: {{ salt.yav.get('ver-01e07mp75ebx4wzwtzws2snkg0[password]') }}
                max_open_conn: 32
                max_idle_conn: 32
    pg_ssl_balancer: mdb-secrets.db.yandex.net
    certs:
        readonly: True


include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - mdb_controlplane_porto_prod.common.solomon
    - mdb_controlplane_porto_prod.common.deploy
    - porto.prod.dbaas.jaeger_agent

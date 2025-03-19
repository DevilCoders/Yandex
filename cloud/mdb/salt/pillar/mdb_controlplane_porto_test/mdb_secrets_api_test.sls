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
        - components.monrun2
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.yasmagent
        - components.tvmtool
        - components.dbaas-porto-controlplane
    deploy:
        version: 2
        api_host: deploy-api-test.db.yandex-team.ru
    tvmtool:
        port: 50001
        tvm_id: 2011060
        config:
            client: mdb-secrets
            secret: {{ salt.yav.get('ver-01dvz3k2xp4ha40dtvg8mdeehk[secret]') }}
        token: {{ salt.yav.get('ver-01dvz3k2xp4ha40dtvg8mdeehk[token]') }}
    mdb-secrets:
        server_name:
            mdb-secrets-test.db.yandex.net
        config:
            app:
                sentry:
                    dsn: {{ salt.yav.get('ver-01f9kv20wppcvnddsre9ckxvbj[dsn]') }}
            saltkey: {{ salt.yav.get('ver-01dvz432ffrf7fpaxnv44rtsrk[key]') }}
            privatekey: {{ salt.yav.get('ver-01dvz44rmaj4cmajnwa51g8h57[key]') }}
            certificate_to_use: yandex
            yandex_crt:
                abc: 2660
                oauth: {{ salt.yav.get('ver-01dvz46ace8xwpwn3nh8r67gv0[token]') }}
            secretsdb:
                addrs:
                 - secrets-db-test01h.db.yandex.net:6432
                 - secrets-db-test01f.db.yandex.net:6432
                 - secrets-db-test01k.db.yandex.net:6432
                user: secrets_api
                password: {{ salt.yav.get('ver-01dvz3ttrbcphd33y41gr9q3kq[password]') }}
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:767:0:c
    solomon:
        project: internal-mdb
        cluster: secrets-api-test
        service: mdb
        ca_path: True
        push_url: https://solomon.yandex-team.ru/api/v2/push
    common:
        dh: |
            {{ salt.yav.get('ver-01dvz3npgcb52d5wp0wrjzqa01[key]') | indent(12) }}
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
        use_yasmagent: False
    pg_ssl_balancer: mdb-secrets-test.db.yandex.net
    certs:
        readonly: True

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.selfdns
    - porto.test.dbaas.jaeger_agent
    - porto.test.dbaas.solomon

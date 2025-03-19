data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dvwke8a6kdgrvn0ttd2gr2wv[token]') | indent(12) }}
    tvmtool:
        token: {{ salt.yav.get('ver-01dvwkm3ep6cqqj6effxnx74vh[token]') }}
        config:
            client: mdb-deploy-api
            secret: {{ salt.yav.get('ver-01dvwkm3ep6cqqj6effxnx74vh[secret]') }}
    mdb-deploy-api:
        server_name: deploy-api-test.db.yandex-team.ru
        config:
            sentry:
                dsn: {{ salt.yav.get('ver-01e7q8x5n30gnt6d8c6ajqq0e5[dsn]') }}
            deployapi:
                master_default_public_key: {{ salt.yav.get('ver-01dvchg68zhj4jeep09xwqbmaw[public]') | yaml_encode }}
                master_max_tasks_per_thread: 2
                saltapi:
                    user: mdb-deploy-salt-api
                    password: {{ salt.yav.get('ver-01dvqg2wjt5a3tvmdmvmr462h4[password]') }}
                    eauth: pam
            deploydb:
                postgresql:
                    hosts:
                        - deploy-db-test01h.db.yandex.net:6432
                        - deploy-db-test01k.db.yandex.net:6432
                        - deploy-db-test01f.db.yandex.net:6432
                    db: deploydb
                    user: deploy_api
                    password: {{ salt.yav.get('ver-01dvwm7syf70bqt1a4z6k6ekev[password]') }}
                    max_open_conn: 64
                    max_idle_conn: 64
        tls_key: |
            {{ salt.yav.get('ver-01dvwmfvec8hrpnr0yg7mg2b3p[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01dvwmfvec8hrpnr0yg7mg2b3p[cert]') | indent(12) }}
    mdb-deploy-cleaner:
        config:
            deploydb:
                postgresql:
                    hosts:
                        - deploy-db-test01h.db.yandex.net:6432
                        - deploy-db-test01k.db.yandex.net:6432
                        - deploy-db-test01f.db.yandex.net:6432
                    db: deploydb
                    user: deploy_cleaner
                    password: {{ salt.yav.get('ver-01e3mewn303cn9har20bcnwjjk[password]') }}

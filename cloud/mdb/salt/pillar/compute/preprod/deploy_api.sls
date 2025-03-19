data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dwh1wj2ffhbaz36ys3mhtqfd[private]') | indent(12) }}
    tvmtool:
        token: {{ salt.yav.get('ver-01dwh20eavg4mh32pczbknw767[token]') }}
        config:
            client: mdb-deploy-api
            secret: {{ salt.yav.get('ver-01dwh20eavg4mh32pczbknw767[secret]') }}
    mdb-deploy-api:
        server_name: mdb-deploy-api.private-api.cloud-preprod.yandex.net
        config:
            sentry:
                dsn: {{ salt.yav.get('ver-01e7q9677xjafxjr48q5twq4jg[dsn]') }}
            deployapi:
                master_default_public_key: {{ salt.yav.get('ver-01dwh5d3bm27xa5gakgfjh30e0[public]') | yaml_encode }}
                saltapi:
                    user: mdb-deploy-salt-api
                    password: {{ salt.yav.get('ver-01dwh26wcxdtrdqr78ssw7z8mv[password]') }}
                    eauth: pam
            deploydb:
                postgresql:
                    hosts:
                        - mdb-deploy-db-preprod01f.cloud-preprod.yandex.net:6432
                        - mdb-deploy-db-preprod01h.cloud-preprod.yandex.net:6432
                        - mdb-deploy-db-preprod01k.cloud-preprod.yandex.net:6432
                    db: deploydb
                    user: deploy_api
                    password: {{ salt.yav.get('ver-01dwh2azbpm53bae023jeygedy[password]') }}
                    max_open_conn: 64
                    max_idle_conn: 64
        tls_key: |
                {{ salt.yav.get('ver-01dwh2mc8apsqn7b1f24g7dmqh[private]') | indent(16) }}
        tls_crt: |
                {{ salt.yav.get('ver-01dwh2mc8apsqn7b1f24g7dmqh[cert]') | indent(16) }}
    mdb-deploy-cleaner:
        config:
            deploydb:
                postgresql:
                    hosts:
                        - mdb-deploy-db-preprod01f.cloud-preprod.yandex.net:6432
                        - mdb-deploy-db-preprod01h.cloud-preprod.yandex.net:6432
                        - mdb-deploy-db-preprod01k.cloud-preprod.yandex.net:6432
                    db: deploydb
                    user: deploy_cleaner
                    password: {{ salt.yav.get('ver-01e3q267zpx60qw2qpd8v3rcm0[password]') }}

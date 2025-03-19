data:
    common:
        dh: |
            {{ salt.yav.get('ver-01e48j3xmd46w2fy44mtf7xkyj[private]') | indent(12) }}
    tvmtool:
        token: {{ salt.yav.get('ver-01e48k0rc48hh24a0wt63w5g7t[token]') }}
        config:
            client: mdb-deploy-api
            secret: {{ salt.yav.get('ver-01e48k0rc48hh24a0wt63w5g7t[secret]') }}
    mdb-deploy-api:
        server_name: mdb-deploy-api.private-api.yandexcloud.net
        config:
            app:
                logging:
                    level: info
            sentry:
                dsn: {{ salt.yav.get('ver-01e7q99xfkpgmz58npcg103jg7[dsn]') }}
            deployapi:
                master_default_public_key: {{ salt.yav.get('ver-01e95gxzpjd6w2ywhxxqmevqn5[public]') | yaml_encode }}
                master_max_tasks_per_thread: 5
                saltapi:
                    user: mdb-deploy-salt-api
                    password: {{ salt.yav.get('ver-01e48ptqtqp5xgj86y9b41djwn[password]') }}
                    eauth: pam
            deploydb:
                postgresql:
                    hosts:
                        - mdb-deploy-db01f.yandexcloud.net:6432
                        - mdb-deploy-db01h.yandexcloud.net:6432
                        - mdb-deploy-db01k.yandexcloud.net:6432
                    db: deploydb
                    user: deploy_api
                    password: {{ salt.yav.get('ver-01e48pznw2zxv12hyk5c7pge83[password]') }}
                    max_open_conn: 64
                    max_idle_conn: 64
        tls_key: |
            {{ salt.yav.get('ver-01e48j8ya9s6c5jdfwxqzxr3hg[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01e48j8ya9s6c5jdfwxqzxr3hg[cert]') | indent(12) }}
    mdb-deploy-cleaner:
        config:
            deploydb:
                postgresql:
                    hosts:
                        - mdb-deploy-db01f.yandexcloud.net:6432
                        - mdb-deploy-db01h.yandexcloud.net:6432
                        - mdb-deploy-db01k.yandexcloud.net:6432
                    db: deploydb
                    user: deploy_cleaner
                    password: {{ salt.yav.get('ver-01e48q1ayk2j7ahr2wh0gyg06t[password]') }}

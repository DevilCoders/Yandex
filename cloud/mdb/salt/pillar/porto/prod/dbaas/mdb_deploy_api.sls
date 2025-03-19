data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dz62yqdhwvhwkeppa6z1rzjr[secret]') | indent(12) }}
    tvmtool:
        token: {{ salt.yav.get('ver-01dz6336hng8nk6gr9n26ga6pj[token]') }}
        config:
            client: mdb-deploy-api
            secret: {{ salt.yav.get('ver-01dz6336hng8nk6gr9n26ga6pj[secret]') }}
    mdb-deploy-api:
        server_name: deploy-api.db.yandex-team.ru
        config:
            app:
                logging:
                    level: info
            sentry:
                dsn: {{ salt.yav.get('ver-01e7q91kpg1t5982smpbe5cb4m[dsn]') }}
            deployapi:
                master_default_public_key: {{ salt.yav.get('ver-01dz8m1pzmh1tmw8s75mygdvnf[public]') | yaml_encode }}
                master_max_tasks_per_thread: 2
                skip_shipments_fqdn_regexes:
                    - '"^man.*"'
                    - '".*01i.db.yandex.net"'
                saltapi:
                    user: mdb-deploy-salt-api
                    password: {{ salt.yav.get('ver-01dz63829xnskw386s17kjv8g8[password]') }}
                    eauth: pam
            deploydb:
                postgresql:
                    hosts:
                        - deploy-db01h.db.yandex.net:6432
                        - deploy-db01k.db.yandex.net:6432
                        - deploy-db01f.db.yandex.net:6432
                    db: deploydb
                    user: deploy_api
                    password: {{ salt.yav.get('ver-01dz63dpeqvax928rt8jj9b70c[password]') }}
                    max_open_conn: 64
                    max_idle_conn: 64
        tls_key: |
            {{ salt.yav.get('ver-01dz63hgrge34rew3m1pdy2mwa[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01dz63hgrge34rew3m1pdy2mwa[cert]') | indent(12) }}
    mdb-deploy-cleaner:
        config:
            deploydb:
                postgresql:
                    hosts:
                        - deploy-db01h.db.yandex.net:6432
                        - deploy-db01k.db.yandex.net:6432
                        - deploy-db01f.db.yandex.net:6432
                    db: deploydb
                    user: deploy_cleaner
                    password: {{ salt.yav.get('ver-01e3q1v05gefhpxs0sap3kqtg4[password]') }}

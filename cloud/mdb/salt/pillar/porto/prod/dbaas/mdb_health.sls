data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dzny2rnmt03edw8cdsszv7fk[secret]') | indent(12) }}
    redis:
        config:
            requirepass: {{ salt.yav.get('ver-01dznye8fqt12jr9k59dka4gff[password]') }}
            masterauth: {{ salt.yav.get('ver-01dznye8fqt12jr9k59dka4gff[password]') }}
        secrets:
            renames:
                CONFIG: {{ salt.yav.get('ver-01dznymadf2j8cxww40v5tg6f2[CONFIG]') }}
                SLAVEOF: {{ salt.yav.get('ver-01dznymadf2j8cxww40v5tg6f2[SLAVEOF]') }}
                DEBUG: {{ salt.yav.get('ver-01dznymadf2j8cxww40v5tg6f2[DEBUG]') }}
                MONITOR: {{ salt.yav.get('ver-01dznymadf2j8cxww40v5tg6f2[MONITOR]') }}
                SHUTDOWN: {{ salt.yav.get('ver-01dznymadf2j8cxww40v5tg6f2[SHUTDOWN]') }}
            sentinel_renames:
                FAILOVER: {{ salt.yav.get('ver-01dznymadf2j8cxww40v5tg6f2[FAILOVER]') }}
                RESET: {{ salt.yav.get('ver-01dznymadf2j8cxww40v5tg6f2[RESET]') }}
    mdb-health:
        server_name: health.db.yandex.net
        sentry:
            dsn: {{ salt.yav.get('ver-01f0bdd25mjyq0k3t3pwkpajes[dsn]') }}
        gomaxprocs: 14
        config:
            secretsstore:
                postgresql:
                    hosts:
                        - meta01f.db.yandex.net:6432
                        - meta01h.db.yandex.net:6432
                        - meta01k.db.yandex.net:6432
                    db: dbaas_metadb
                    user: mdb_health
                    password: {{ salt.yav.get('ver-01dznyrcya1qp4rynb475kjxbh[password]') }}
                    max_open_conn: 64
                    max_idle_conn: 64
            datastore:
                redis:
                    hosts:
                        - health01h.db.yandex.net:26379
                        - health01k.db.yandex.net:26379
                        - health01f.db.yandex.net:26379
                    mastername: mymaster
                    db: 0
                    password: {{ salt.yav.get('ver-01dznye8fqt12jr9k59dka4gff[password]') }}
                    client:
                        pool_size: 200
                        min_idle_conns: 200

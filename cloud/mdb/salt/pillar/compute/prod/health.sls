data:
    common:
        dh: |
            {{ salt.yav.get('ver-01e39qsemzsa9jef5mvm3e2c3j[private]') | indent(12) }}
    redis:
        config:
            requirepass: {{ salt.yav.get('ver-01e39qwa8r86271qe61jzb4jqn[password]') }}
            masterauth: {{ salt.yav.get('ver-01e39qwa8r86271qe61jzb4jqn[password]') }}
        secrets:
            renames:
                CONFIG: {{ salt.yav.get('ver-01e39r06v37py6b7t47hckcj8m[CONFIG]') }}
                SLAVEOF: {{ salt.yav.get('ver-01e39r06v37py6b7t47hckcj8m[SLAVEOF]') }}
                DEBUG: {{ salt.yav.get('ver-01e39r06v37py6b7t47hckcj8m[DEBUG]') }}
                MONITOR: {{ salt.yav.get('ver-01e39r06v37py6b7t47hckcj8m[MONITOR]') }}
                SHUTDOWN: {{ salt.yav.get('ver-01e39r06v37py6b7t47hckcj8m[SHUTDOWN]') }}
            sentinel_renames:
                FAILOVER: {{ salt.yav.get('ver-01e39r06v37py6b7t47hckcj8m[FAILOVER]') }}
                RESET: {{ salt.yav.get('ver-01e39r06v37py6b7t47hckcj8m[RESET]') }}
    mdb-health:
        server_name: mdb-health.private-api.cloud.yandex.net
        sentry:
            dsn: {{ salt.yav.get('ver-01f0bdhxtzaq98rqjgny65qm41[dsn]') }}
        gomaxprocs: 14
        config:
            secretsstore:
                postgresql:
                    hosts:
                        - meta-dbaas01f.yandexcloud.net:6432
                        - meta-dbaas01h.yandexcloud.net:6432
                        - meta-dbaas01k.yandexcloud.net:6432
                    db: dbaas_metadb
                    user: mdb_health
                    password: {{ salt.yav.get('ver-01e39r3wdwc5c5bqdva1r1c4q0[password]') }}
                    max_open_conn: 64
                    max_idle_conn: 64
            datastore:
                redis:
                    hosts:
                        - health-dbaas01f.yandexcloud.net:26379
                        - health-dbaas01h.yandexcloud.net:26379
                        - health-dbaas01k.yandexcloud.net:26379
                    mastername: mdb-health-compute-prod
                    db: 0
                    password: {{ salt.yav.get('ver-01e39qwa8r86271qe61jzb4jqn[password]') }}
                    client:
                        pool_size: 200
                        min_idle_conns: 200

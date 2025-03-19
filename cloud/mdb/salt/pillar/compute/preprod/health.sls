data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dws5615we939cy3ybq5d4adr[private]') | indent(12) }}
    redis:
        config:
            requirepass: {{ salt.yav.get('ver-01dws5990tkv7m7ya9g4f8rhtf[password]') }}
            masterauth: {{ salt.yav.get('ver-01dws5990tkv7m7ya9g4f8rhtf[password]') }}
        secrets:
            renames:
                CONFIG: {{ salt.yav.get('ver-01dws5dntadzppww26j9bt9pma[CONFIG]') }}
                SLAVEOF: {{ salt.yav.get('ver-01dws5dntadzppww26j9bt9pma[SLAVEOF]') }}
                DEBUG: {{ salt.yav.get('ver-01dws5dntadzppww26j9bt9pma[DEBUG]') }}
                MONITOR: {{ salt.yav.get('ver-01dws5dntadzppww26j9bt9pma[MONITOR]') }}
                SHUTDOWN: {{ salt.yav.get('ver-01dws5dntadzppww26j9bt9pma[SHUTDOWN]') }}
            sentinel_renames:
                FAILOVER: {{ salt.yav.get('ver-01dws5dntadzppww26j9bt9pma[FAILOVER]') }}
                RESET: {{ salt.yav.get('ver-01dws5dntadzppww26j9bt9pma[RESET]') }}
    mdb-health:
        server_name: mdb-health.private-api.cloud-preprod.yandex.net
        sentry:
            dsn: {{ salt.yav.get('ver-01f0bdmrcq0chj413rjxwev09n[dsn]') }}
        config:
            logging:
                level: debug
            secretsstore:
                postgresql:
                    hosts:
                        - meta-dbaas-preprod01f.cloud-preprod.yandex.net:6432
                        - meta-dbaas-preprod01h.cloud-preprod.yandex.net:6432
                        - meta-dbaas-preprod01k.cloud-preprod.yandex.net:6432
                    db: dbaas_metadb
                    user: mdb_health
                    password: {{ salt.yav.get('ver-01dws5kr0djdeeg5zjz20e70xd[password]') }}
                    max_open_conn: 64
                    max_idle_conn: 64
            datastore:
                redis:
                    hosts:
                        - health-dbaas-preprod01f.cloud-preprod.yandex.net:26379
                        - health-dbaas-preprod01h.cloud-preprod.yandex.net:26379
                        - health-dbaas-preprod01k.cloud-preprod.yandex.net:26379
                    mastername: mdb-health-compute-preprod
                    db: 0
                    password: {{ salt.yav.get('ver-01dws5990tkv7m7ya9g4f8rhtf[password]') }}

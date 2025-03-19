data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dvxb68kqmkpkrkkpdw8czk2a[token]') | indent(12) }}
    redis:
        config:
            requirepass: {{ salt.yav.get('ver-01dvxbdwmcxcz9asp7m7fgemnd[pass]') }}
            masterauth: {{ salt.yav.get('ver-01dvxbdwmcxcz9asp7m7fgemnd[pass]') }}
        secrets:
            renames:
                CONFIG: {{ salt.yav.get('ver-01dvxbmhg4sc2ab236f3s1hdkp[CONFIG]') }}
                SLAVEOF: {{ salt.yav.get('ver-01dvxbmhg4sc2ab236f3s1hdkp[SLAVEOF]') }}
                DEBUG: {{ salt.yav.get('ver-01dvxbmhg4sc2ab236f3s1hdkp[DEBUG]') }}
                MONITOR: {{ salt.yav.get('ver-01dvxbmhg4sc2ab236f3s1hdkp[MONITOR]') }}
                SHUTDOWN: {{ salt.yav.get('ver-01dvxbmhg4sc2ab236f3s1hdkp[SHUTDOWN]') }}
            sentinel_renames:
                FAILOVER: {{ salt.yav.get('ver-01dvxbmhg4sc2ab236f3s1hdkp[FAILOVER]') }}
                RESET: {{ salt.yav.get('ver-01dvxbmhg4sc2ab236f3s1hdkp[RESET]') }}
    mdb-health:
        sentry:
            dsn: {{ salt.yav.get('ver-01f0b6grnmwcp1tyv23t618mkj[dsn]') }}
        server_name: mdb-health-test.db.yandex.net
        config:
            logging:
                level: debug
            secretsstore:
                postgresql:
                    hosts:
                        - meta-test01f.db.yandex.net:6432
                        - meta-test01h.db.yandex.net:6432
                        - meta-test01k.db.yandex.net:6432
                    db: dbaas_metadb
                    user: mdb_health
                    password: {{ salt.yav.get('ver-01dvxc1g5w19qsr7z0psfjbm49[password]') }}
                    max_open_conn: 64
                    max_idle_conn: 64
            datastore:
                redis:
                    hosts:
                        - health-test01h.db.yandex.net:26379
                        - health-test01k.db.yandex.net:26379
                        - health-test01f.db.yandex.net:26379
                    mastername: mymaster
                    db: 0
                    password: {{ salt.yav.get('ver-01dvxbdwmcxcz9asp7m7fgemnd[pass]') }}
                    client:
                        read_timeout: 5s
                        pool_size: 100
            clickhouse:
                addrs:
                    - "man-pafknhg5ejz7mswp.db.yandex.net:9440"
                    - "sas-8tibsna8ps6el3tz.db.yandex.net:9440"
                    - "vla-o6fwmt7x8qucxwjj.db.yandex.net:9440"
                password: {{ salt.yav.get('ver-01fx87e78tt0hgabp5tfbynjme[password]') }}

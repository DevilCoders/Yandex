data:
    mdb-dataproc-manager:
        server:
            listen: 0.0.0.0:50051
        config:
            logging:
                url: api.cloud.yandex.net:443
                log_group_id: e238k014i53vkicv4npg
            logging_service_account_key: {{ salt.yav.get('sec-01fjf1wfne2cpkn2vkye28brwz[key]') }}
            service_account:
                id: ajefj4rf3598dbojbaue
                key_id: ajenf1qhdadbb7lpg0s2
                private_key: |
                    {{ salt.yav.get('ver-01f2tkqc1s5tbtfjnvkmyf516f[ private_key ]') | indent(20) }}
            tls:
                cert_file: {{ salt.yav.get('ver-01e2g68mkdpy51q2mgrwbjfn6h[cert]') }}
                key_file: {{ salt.yav.get('ver-01e2g68mkdpy51q2mgrwbjfn6h[key]') }}
            internal-api:
                url: https://mdb.private-api.cloud.yandex.net/
                access_id: '9156e5e5-ec3f-4a02-9d95-9cc74aaf98d0'
                access_secret: {{ salt.yav.get('ver-01e2g7279c4dvage0nysecpg02[secret]') }}
            sentry:
                dsn: {{ salt.yav.get('ver-01e3vhbz9567qgtyk6j17yygc9[dsn]') }}
                environment: compute-prod
            compute-service:
                service_url: compute-api.cloud.yandex.net:9051
                token: {{ salt.yav.get('ver-01f32rasr2q3baccafesax2gqy[token]') }}
                grpc_timeout: 30s
            instance-group-service:
                service_url: instance-group.private-api.ycp.cloud.yandex.net:443
                token: {{ salt.yav.get('ver-01f32rasr2q3baccafesax2gqy[token]') }}
                grpc_timeout: 30s
            iam-token-service:
                service_url: ts.private-api.cloud.yandex.net:4282
                token: {{ salt.yav.get('ver-01f32rasr2q3baccafesax2gqy[token]') }}
                grpc_timeout: 30s
                token_cache_timeout: 6h
        redis:
            hosts:
                - dataproc-manager01f.yandexcloud.net:26379
                - dataproc-manager01h.yandexcloud.net:26379
                - dataproc-manager01k.yandexcloud.net:26379
            mastername: mdb-dataproc-manager-redis-prod
            db: 0
            password: '{{ salt.yav.get('ver-01e2g6bzfcaaat427jzv4dbdct[password]') }}'
    redis:
        config:
            requirepass: '{{ salt.yav.get('ver-01e2g6bzfcaaat427jzv4dbdct[password]') }}'
            masterauth: '{{ salt.yav.get('ver-01e2g6bzfcaaat427jzv4dbdct[password]') }}'
        secrets:
            renames:
                CONFIG: {{ salt.yav.get('ver-01e2g6fss6sabzppt0cjsxwwfm[CONFIG]') }}
                SLAVEOF: {{ salt.yav.get('ver-01e2g6fss6sabzppt0cjsxwwfm[SLAVEOF]') }}
                DEBUG: {{ salt.yav.get('ver-01e2g6fss6sabzppt0cjsxwwfm[DEBUG]') }}
                MONITOR: {{ salt.yav.get('ver-01e2g6fss6sabzppt0cjsxwwfm[MONITOR]') }}
                SHUTDOWN: {{ salt.yav.get('ver-01e2g6fss6sabzppt0cjsxwwfm[SHUTDOWN]') }}
            sentinel_renames:
                FAILOVER: {{ salt.yav.get('ver-01e2g6fss6sabzppt0cjsxwwfm[FAILOVER]') }}
                RESET: {{ salt.yav.get('ver-01e2g6fss6sabzppt0cjsxwwfm[RESET]') }}
    mdb_metrics:
        dataproc_health:
            metadb_password: {{ salt.yav.get('ver-01eh7d97qnd0v7e26q05jb2ske[password]') }}
            metadb_hosts:
                - meta-dbaas01f.yandexcloud.net
                - meta-dbaas01h.yandexcloud.net
                - meta-dbaas01k.yandexcloud.net
    envoy:
        use_health_map: true
        clusters:
            dataproc-manager:
                prefix: "/"
                port: 50051

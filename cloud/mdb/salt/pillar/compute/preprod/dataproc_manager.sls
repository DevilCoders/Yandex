data:
    mdb-dataproc-manager:
        server:
            listen: 0.0.0.0:50051
        config:
            logging:
                url: api.cloud-preprod.yandex.net:443
                log_group_id: af3hnjiadl05hprh0k0d
            logging_service_account_key: {{ salt.yav.get('sec-01fjf02c9x0ck91fdr6k31ghty[key]') }}
            service_account:
                id: bfbf93hbuj25evok8696
                key_id: bfbu8fgi30ahqookpkcb
                private_key: |
                    {{ salt.yav.get('ver-01f2tk896ht5cr8ybx618s64qp[ private_key ]') | indent(20) }}
            tls:
                key_file: {{ salt.yav.get('ver-01dwa8wkeptermjjfxsb64geh9[private]') }}
                cert_file: {{ salt.yav.get('ver-01dwa8wkeptermjjfxsb64geh9[cert]') }}
            internal-api:
                url: https://mdb.private-api.cloud-preprod.yandex.net
                access_id: {{ salt.yav.get('ver-01dw7qv4nb107cn5cvy9k7052m[id]') }}
                access_secret: {{ salt.yav.get('ver-01dw7qv4nb107cn5cvy9k7052m[secret]') }}
            sentry:
                dsn: {{ salt.yav.get('ver-01e3vhbz9567qgtyk6j17yygc9[dsn]') }}
                environment: compute-preprod
            compute-service:
                service_url: compute-api.cloud-preprod.yandex.net:9051
                token: {{ salt.yav.get('ver-01f32r6gdh49fm1kxymsc7bj3j[token]') }}
                grpc_timeout: 30s
            instance-group-service:
                service_url: instance-group.private-api.ycp.cloud-preprod.yandex.net:443
                token: {{ salt.yav.get('ver-01f32r6gdh49fm1kxymsc7bj3j[token]') }}
                grpc_timeout: 30s
            iam-token-service:
                service_url: ts.private-api.cloud-preprod.yandex.net:4282
                token: {{ salt.yav.get('ver-01f32r6gdh49fm1kxymsc7bj3j[token]') }}
                grpc_timeout: 30s
                token_cache_timeout: 6h
        redis:
            hosts:
                - dataproc-manager-preprod01f.cloud-preprod.yandex.net:26379
                - dataproc-manager-preprod01h.cloud-preprod.yandex.net:26379
                - dataproc-manager-preprod01k.cloud-preprod.yandex.net:26379
            mastername: mdb-dataproc-manager-redis-preprod
            db: 0
            password: {{ salt.yav.get('ver-01dwa8zsdyfce7c42sdgdns94j[password]') }}
    redis:
        config:
            requirepass: {{ salt.yav.get('ver-01dwa8zsdyfce7c42sdgdns94j[password]') }}
            masterauth: {{ salt.yav.get('ver-01dwa8zsdyfce7c42sdgdns94j[password]') }}
        secrets:
            renames:
                CONFIG: {{ salt.yav.get('ver-01dwa9afb816nd1av6cwjdbjkf[CONFIG]') }}
                SLAVEOF: {{ salt.yav.get('ver-01dwa9afb816nd1av6cwjdbjkf[SLAVEOF]') }}
                DEBUG: {{ salt.yav.get('ver-01dwa9afb816nd1av6cwjdbjkf[DEBUG]') }}
                MONITOR: {{ salt.yav.get('ver-01dwa9afb816nd1av6cwjdbjkf[MONITOR]') }}
                SHUTDOWN: {{ salt.yav.get('ver-01dwa9afb816nd1av6cwjdbjkf[SHUTDOWN]') }}
            sentinel_renames:
                FAILOVER: {{ salt.yav.get('ver-01dwa9afb816nd1av6cwjdbjkf[FAILOVER]') }}
                RESET: {{ salt.yav.get('ver-01dwa9afb816nd1av6cwjdbjkf[RESET]') }}
    mdb_metrics:
        dataproc_health:
            metadb_password: {{ salt.yav.get('ver-01eh259jc4jp1akryjhr16aaf8[password]') }}
            metadb_hosts:
                - meta-dbaas-preprod01f.cloud-preprod.yandex.net
                - meta-dbaas-preprod01h.cloud-preprod.yandex.net
                - meta-dbaas-preprod01k.cloud-preprod.yandex.net
    envoy:
        use_health_map: true
        clusters:
            dataproc-manager:
                prefix: "/"
                port: 50051

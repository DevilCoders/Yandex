data:
    common:
        dh: |
            {{ salt.yav.get('ver-01e2g7v95aftkrbphhsjyrh5c7[private]') | indent(12) }}
    dnsapi:
        slayer:
            baseurl: "https://dns-api.yandex.net"
            account: robot-dnsapi-mdb
            token: {{ salt.yav.get('ver-01e2g83gr0n446z8xrrgynea33[token]') }}
            fqdnsuffix: mdb.yandexcloud.net
        compute:
            grpcurl: network-api.private-api.cloud.yandex.net:9823
            token: {{ salt.yav.get('ver-01e2g8jwpen7758x2qyv0v0ajv[token]') }}
            fqdnsuffix: mdb.yandexcloud.net
        service_account:
            id: {{ salt.yav.get('ver-01er49kk6maqc92t1r7704egwm[id]') }}
            key_id: {{ salt.yav.get('ver-01er49kk6maqc92t1r7704egwm[key_id]') }}
            private_key: {{ salt.yav.get('ver-01er49kk6maqc92t1r7704egwm[private_key]') | yaml_encode }}
    secretsstore:
        postgresql:
            hosts:
                - meta-dbaas01f.yandexcloud.net:6432
                - meta-dbaas01h.yandexcloud.net:6432
                - meta-dbaas01k.yandexcloud.net:6432
            db: dbaas_metadb
            user: mdb_dns
            password: {{ salt.yav.get('ver-01e2g8rtedva84vq061jyedr6a[password]') }}
    mdb-dns:
        server_name: mdb-dns.private-api.cloud.yandex.net
        tls_key: |
            {{ salt.yav.get('ver-01e2g7xzte710kb38xp30p1wca[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01e2g7xzte710kb38xp30p1wca[cert]') | indent(12) }}

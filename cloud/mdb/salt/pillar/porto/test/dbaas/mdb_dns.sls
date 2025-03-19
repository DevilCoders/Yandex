data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dvwzc3n916v1v8rk6h36dycp[token]') | indent(12) }}
    secretsstore:
        postgresql:
            hosts:
                - meta-test01f.db.yandex.net:6432
                - meta-test01h.db.yandex.net:6432
                - meta-test01k.db.yandex.net:6432
            db: dbaas_metadb
            user: mdb_dns
            password: '{{ salt.yav.get('ver-01dvwzj3crybgdmwv3veqffwpq[password]') }}'
    dnsapi:
        slayer:
            baseurl: "https://dns-api.yandex.net"
            account: robot-dnsapi-mdb
            token: {{ salt.yav.get('ver-01dvwzt78r6wysqtkdmmwgpxca[token]') }}
            fqdnsuffix: db.yandex.net
        service_account:
            id: ""
            key_id: ""
            private_key: ""
    mdb-dns:
        server_name: mdb-dns-test.db.yandex.net
        tls_key: |
                {{ salt.yav.get('ver-01dvwzy4yex6d220kz2sgaxgpj[key]') | indent(16) }}
        tls_crt: |
                {{ salt.yav.get('ver-01dvwzy4yex6d220kz2sgaxgpj[cert]') | indent(16) }}

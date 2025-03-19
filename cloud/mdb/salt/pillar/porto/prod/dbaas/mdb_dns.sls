data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dznxamqe1rqx0hzpryrwgptn[private]') | indent(12) }}
    secretsstore:
        postgresql:
            hosts:
                - meta01f.db.yandex.net:6432
                - meta01h.db.yandex.net:6432
                - meta01k.db.yandex.net:6432
            db: dbaas_metadb
            user: mdb_dns
            password: {{ salt.yav.get('ver-01dznxsm22fzpwmpscraew4e80[password]') }}
    dnsapi:
        slayer:
            baseurl: "https://dns-api.yandex.net"
            account: robot-dnsapi-mdb
            token: {{ salt.yav.get('ver-01dz8psx0knrbadd4p97gd2dq1[token]') }}
            fqdnsuffix: db.yandex.net
        service_account:
            id: ""
            key_id: ""
            private_key: ""
    mdb-dns:
        server_name: mdb-dns.db.yandex.net
        tls_key: |
            {{ salt.yav.get('ver-01dznxfy2c3tws4dzdfvvby4wq[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01dznxfy2c3tws4dzdfvvby4wq[cert]') | indent(12) }}

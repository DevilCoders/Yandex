data:
    common:
        dh: |
            {{ salt.yav.get('ver-01e4bn8fx19wx2d3c6kh4tkd7b[private]') | indent(12) }}
    tvmtool:
        token: {{ salt.yav.get('ver-01e4dbb64ks61n38ze31k7y2g3[token]') }}
        config:
            client: mdb-cms-test
            secret: {{ salt.yav.get('ver-01e48wym30m5kwjxsf9xxf0h56[client_secret]') }}
    mdb-cms:
        config:
            metadb:
                addrs:
                    - meta-test01f.db.yandex.net:6432
                    - meta-test01h.db.yandex.net:6432
                    - meta-test01k.db.yandex.net:6432
                password: {{ salt.yav.get('ver-01e43k7em77zrmx608bmzjr1xz[password]') }}
            cmsdb:
                addrs:
                    - cms-db-test01f.db.yandex.net:6432
                    - cms-db-test01h.db.yandex.net:6432
                    - cms-db-test01k.db.yandex.net:6432
                password: {{ salt.yav.get('ver-01e43k7em77zrmx608bmzjr1xz[password]') }}
        server_name: mdb-cms-test.db.yandex.net
    pg_ssl_balancer: mdb-cms-test.db.yandex.net

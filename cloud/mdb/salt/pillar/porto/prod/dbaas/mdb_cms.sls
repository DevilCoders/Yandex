data:
    common:
        dh: |
            {{ salt.yav.get('ver-01e57mz89r3bdct7775fdsb1ng[private]') | indent(12) }}
    tvmtool:
        token: {{ salt.yav.get('ver-01e57q5yr0x2c77jn6jy222wch[token]') }}
        config:
            client: mdb-cms-porto-prod
            secret: {{ salt.yav.get('ver-01e57716dnq15t53zbjnbeks77[client_secret]') }}
    mdb-cms:
        config:
            metadb:
                addrs:
                    - meta01k.db.yandex.net:6432
                    - meta01f.db.yandex.net:6432
                    - meta01h.db.yandex.net:6432
                password: {{ salt.yav.get('ver-01e57r9nz9fa5f0nsp5kbr6dhe[password]') }}
            cmsdb:
                addrs:
                    - cms-db-prod01f.db.yandex.net:6432
                    - cms-db-prod01h.db.yandex.net:6432
                    - cms-db-prod01k.db.yandex.net:6432
                password: {{ salt.yav.get('ver-01e57r9nz9fa5f0nsp5kbr6dhe[password]') }}
        server_name: mdb-cms.db.yandex.net
    pg_ssl_balancer: mdb-cms.db.yandex.net

data:
    config:
        pgusers:
            billing_bookkeeper:
                password: {{ salt.yav.get('ver-01g2d7h91jr6h0dv97cc8d2sva[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'dbaas_metadb'

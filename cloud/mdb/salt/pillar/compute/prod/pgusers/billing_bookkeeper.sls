data:
    config:
        pgusers:
            billing_bookkeeper:
                password: {{ salt.yav.get('ver-01g2d8267tyh8tp24k05j9ds4w[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'dbaas_metadb'

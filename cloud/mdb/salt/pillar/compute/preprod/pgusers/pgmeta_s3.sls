data:
    config:
        pgusers:
            s3ro:
                password: {{ salt.yav.get('ver-01eqtd5v3vzwpc5x7vsmde08st[s3ro]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 300
                connect_dbs:
                    - s3db
                pgpass: True
                port: 6432

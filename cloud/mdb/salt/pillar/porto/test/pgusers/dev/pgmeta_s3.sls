data:
    config:
        pgusers:
            s3ro:
                password: {{ salt.yav.get('ver-01e984jfx3cd5wfw0257yg6qee[password]') }}
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

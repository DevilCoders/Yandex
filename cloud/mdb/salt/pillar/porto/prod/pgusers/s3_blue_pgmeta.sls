data:
    config:
        pgusers:
            pgproxy:
                password: {{ salt.yav.get('ver-01fphs2crgjwm6gcaf20nx5n2w[pgproxy]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - '*'
                pgpass: True

            s3ro:
                password: {{ salt.yav.get('ver-01fphs2crgjwm6gcaf20nx5n2w[s3ro]') }}
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

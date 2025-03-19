data:
    config:
        pgusers:
            pgproxy:
                password: {{ salt.yav.get('ver-01ecpsnkyzz4jd0d1b6jtk6s94[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: False
                bouncer: False
                connect_dbs:
                    - '*'

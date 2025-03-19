data:
    config:
        pgusers:
            pgproxy:
                password: {{ salt.yav.get('ver-01e1c349qrx5c7678nh1z9e82b[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: False
                bouncer: False
                connect_dbs:
                    - '*'

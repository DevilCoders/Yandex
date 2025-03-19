data:
    config:
        pgusers:
            pgproxy:
                password: {{ salt.yav.get('ver-01e2056dd8d8536xnyh6tfevg2[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: False
                bouncer: False
                connect_dbs:
                    - '*'

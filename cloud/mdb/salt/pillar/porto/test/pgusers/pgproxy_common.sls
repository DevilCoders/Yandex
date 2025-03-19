data:
    config:
        pgusers:
            pgproxy:
                password: {{ salt.yav.get('ver-01dw72myhnx1xpz9k9z4pks0j5[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: False
                bouncer: False
                connect_dbs:
                    - '*'

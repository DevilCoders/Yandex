data:
    config:
        pgusers:
            pgproxy:
                password: {{ salt.yav.get('ver-01esrry7wrh4ykvaf9c1vz2rpe[pgproxy]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: False
                bouncer: False
                connect_dbs:
                    - '*'

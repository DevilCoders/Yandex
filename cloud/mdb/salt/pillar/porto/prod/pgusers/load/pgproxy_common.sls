data:
    config:
        pgusers:
            pgproxy:
                password: {{ salt.yav.get('ver-01e11h75s9bmgry6v3x8b12g3c[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: False
                bouncer: False
                connect_dbs:
                    - '*'

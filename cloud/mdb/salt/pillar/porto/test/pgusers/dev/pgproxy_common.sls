data:
    config:
        pgusers:
            pgproxy:
                password: {{ salt.yav.get('ver-01e984em4nrdktvwe4vfbvybdj[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: False
                bouncer: False
                connect_dbs:
                    - '*'

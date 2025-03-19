data:
    config:
        pgusers:
            salt_event_reporter:
                password: {{ salt.yav.get('ver-01e983f91fvn9272rm1xmpzjra[password]') }}
                allow_port: '6432'
                allow_db: salt
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - salt

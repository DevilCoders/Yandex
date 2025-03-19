data:
    config:
        pgusers:
            pgproxy:
                allow_db: '*'
                allow_port: '*'
                bouncer: false
                connect_dbs:
                - '*'
                create: false
                password: '{{ salt.lockbox.get("bcnh5hvh0iq5b8nsrefa").password }}'
                replication: false
                superuser: false

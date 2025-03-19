data:
    config:
        pgusers:
            dbm:
                password: {{ salt.yav.get('ver-01e9dw76389zkf0hqyfnedvr2p[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 250
                settings:
                    lock_timeout: '5s'
                connect_dbs:
                    - dbm
                    - salt

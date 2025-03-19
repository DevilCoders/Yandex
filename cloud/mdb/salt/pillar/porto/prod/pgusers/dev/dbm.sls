data:
    config:
        pgusers:
            dbm:
                password: {{ salt.yav.get('ver-01e1xw4g79pc53sxkz3fy70aye[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 50
                settings:
                    lock_timeout: '5s'
                connect_dbs:
                    - dbm
                    - salt

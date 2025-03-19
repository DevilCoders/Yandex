data:
    config:
        pgusers:
            mlock:
                password: {{ salt.yav.get('ver-01ecsx50z8caqd9pdx3x7dk2tg[password]') }}
                allow_port: '6432'
                allow_db: 'mlockdb'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - 'mlockdb'

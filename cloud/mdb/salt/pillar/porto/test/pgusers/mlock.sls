data:
    config:
        pgusers:
            mlock:
                password: {{ salt.yav.get('ver-01e9462fqkx160schpysd1b6ba[password]') }}
                allow_port: '6432'
                allow_db: 'mlockdb'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - 'mlockdb'

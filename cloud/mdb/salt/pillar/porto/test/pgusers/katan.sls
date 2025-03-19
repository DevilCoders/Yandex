data:
    config:
        pgusers:
            katan:
                password: {{ salt.yav.get('ver-01e0zqeqsax240b74wdhhetgep[password]') }}
                allow_port: '6432'
                allow_db: 'katandb'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'katandb'

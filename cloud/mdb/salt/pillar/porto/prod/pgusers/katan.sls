data:
    config:
        pgusers:
            katan:
                password: {{ salt.yav.get('ver-01ecq7gmxmk4krna7abk16sm40[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'katandb'

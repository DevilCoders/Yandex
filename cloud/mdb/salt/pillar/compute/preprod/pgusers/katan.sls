data:
    config:
        pgusers:
            katan:
                password: {{ salt.yav.get('ver-01e6gwrarhtzfc84ek9z7cnqec[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 64
                connect_dbs:
                    - 'katandb'

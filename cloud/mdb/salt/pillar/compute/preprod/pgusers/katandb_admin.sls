data:
    config:
        pgusers:
            katandb_admin:
                password: {{ salt.yav.get('ver-01fr5p3hf2b50c1tnjzja2zymq[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 64
                connect_dbs:
                    - 'katandb'

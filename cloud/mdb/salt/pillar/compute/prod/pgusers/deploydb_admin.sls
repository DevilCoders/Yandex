data:
    config:
        pgusers:
            deploydb_admin:
                password: {{ salt.yav.get('ver-01fpsvdt50kg6ndzj31p3w6eya[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - deploydb

data:
    config:
        pgusers:
            deploydb_admin:
                password: {{ salt.yav.get('ver-01fpsvp0pc5fw7d99x9ze87a5k[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - deploydb

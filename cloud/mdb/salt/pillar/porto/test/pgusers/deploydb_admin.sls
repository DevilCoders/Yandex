data:
    config:
        pgusers:
            deploydb_admin:
                password: {{ salt.yav.get('ver-01fpsvm3aerx7x6epcsryq4xjr[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - deploydb

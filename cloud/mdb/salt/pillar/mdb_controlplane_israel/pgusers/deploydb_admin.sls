data:
    config:
        pgusers:
            deploydb_admin:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 100
                connect_dbs:
                - deploydb
                create: true
                replication: false
                superuser: false

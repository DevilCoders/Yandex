data:
    config:
        pgusers:
            secretsdb_admin:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 100
                connect_dbs:
                - secretsdb
                create: true
                replication: false
                superuser: false

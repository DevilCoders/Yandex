data:
    config:
        pgusers:
            secrets_api:
                allow_db: secretsdb
                allow_port: '6432'
                bouncer: true
                connect_dbs:
                - secretsdb
                create: true
                replication: false
                superuser: false

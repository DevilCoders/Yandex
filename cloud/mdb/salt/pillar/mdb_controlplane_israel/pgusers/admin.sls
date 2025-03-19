data:
    config:
        pgusers:
            admin:
                allow_db: '*'
                allow_port: '*'
                bouncer: false
                connect_dbs:
                - '*'
                create: true
                replication: false
                settings:
                    default_transaction_isolation: read committed
                    log_statement: mod
                superuser: true


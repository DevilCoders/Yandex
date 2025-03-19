data:
    config:
        pgusers:
            monitor:
                allow_db: '*'
                allow_port: '*'
                bouncer: true
                connect_dbs:
                - '*'
                create: true
                replication: false
                settings:
                    default_transaction_isolation: read committed
                    log_statement: mod
                superuser: false


data:
    config:
        pgusers:
            postgres:
                allow_db: '*'
                allow_port: '*'
                bouncer: false
                connect_dbs:
                - '*'
                create: true
                replication: true
                settings:
                    default_transaction_isolation: read committed
                    lock_timeout: 0
                    log_statement: mod
                    synchronous_commit: local
                    temp_file_limit: -1
                superuser: true


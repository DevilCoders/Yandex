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
                password: '{{ salt.lockbox.get("bcn72oant62nud76bl05").password }}'
                replication: false
                settings:
                    default_transaction_isolation: read committed
                    log_statement: mod
                superuser: true
            monitor:
                allow_db: '*'
                allow_port: '*'
                bouncer: true
                connect_dbs:
                - '*'
                create: true
                password: '{{ salt.lockbox.get("bcn6055nr9tnp5ao0rfq").password }}'
                replication: false
                settings:
                    default_transaction_isolation: read committed
                    log_statement: mod
                superuser: false
            postgres:
                allow_db: '*'
                allow_port: '*'
                bouncer: false
                connect_dbs:
                - '*'
                create: true
                password: '{{ salt.lockbox.get("bcnmnb9aedrdu39bhamp").password }}'
                replication: true
                settings:
                    default_transaction_isolation: read committed
                    lock_timeout: 0
                    log_statement: mod
                    synchronous_commit: local
                    temp_file_limit: -1
                superuser: true
            repl:
                allow_db: '*'
                allow_port: '*'
                bouncer: false
                create: true
                password: '{{ salt.lockbox.get("bcn9q0h83vridoubgv68").password }}'
                replication: true
                superuser: false

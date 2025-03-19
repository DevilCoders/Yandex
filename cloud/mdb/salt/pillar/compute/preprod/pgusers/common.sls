data:
    config:
        pgusers:
            monitor:
                password: {{ salt.yav.get('ver-01dwh2vfd6b9kmcnystb0es7z0[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - '*'
                settings:
                    default_transaction_isolation: 'read committed'
                    log_statement: 'mod'
            repl:
                password: {{ salt.yav.get('ver-01dwh2x9km7n4nvpppv1tevag9[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: True
                create: True
                bouncer: False
            admin:
                password: {{ salt.yav.get('ver-01dwh2zrwdywggby8vsfmnfdzy[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: True
                replication: False
                create: True
                bouncer: False
                connect_dbs:
                    - '*'
                settings:
                    default_transaction_isolation: 'read committed'
                    log_statement: 'mod'
            postgres:
                password: {{ salt.yav.get('ver-01dwh30kdt577g3cg2x3h7z5ce[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: True
                replication: True
                create: True
                bouncer: False
                connect_dbs:
                    - '*'
                settings:
                    default_transaction_isolation: 'read committed'
                    lock_timeout: 0
                    log_statement: 'mod'
                    synchronous_commit: 'local'
                    temp_file_limit: -1

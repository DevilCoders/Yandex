data:
    config:
        pgusers:
            monitor:
                password: {{ salt.yav.get('ver-01e1xw09fkj8xnd022z3wp9s14[password]') }}
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
                password: {{ salt.yav.get('ver-01e1xw0w2re9vyppb7qbh7j5nq[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: True
                create: True
                bouncer: False
            admin:
                password: {{ salt.yav.get('ver-01e1xw1zy5grq20m5jqghepd50[password]') }}
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
                password: {{ salt.yav.get('ver-01e1xw2edjmnetryzxdy7ks7rr[password]') }}
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
            monitortmp:
                password: {{ salt.yav.get('ver-01e1xw2x3yrq7w08q0da39whxg[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - '*'

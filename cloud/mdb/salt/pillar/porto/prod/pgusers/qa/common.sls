data:
    config:
        pgusers:
            monitor:
                password: {{ salt.yav.get('ver-01e20509f3gxgwnrn2vq0fq0t9[password]') }}
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
                password: {{ salt.yav.get('ver-01e20510gembs16026t6v8973w[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: True
                create: True
                bouncer: False
            admin:
                password: {{ salt.yav.get('ver-01e205341712f76zv93es92tpt[password]') }}
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
                password: {{ salt.yav.get('ver-01e2053th603rk3angqpg4ye7m[password]') }}
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
                password: {{ salt.yav.get('ver-01e2054dh0dcr206xtznqdma2n[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - '*'

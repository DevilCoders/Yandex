data:
    config:
        pgusers:
            monitor:
                password: {{ salt.yav.get('ver-01dvwxgwtnn0qw3cz2nn0skzxt[password]') }}
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
                password: {{ salt.yav.get('ver-01dvwxkpfya6k1fw3gp2h14b1r[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: True
                create: True
                bouncer: False
            admin:
                password: {{ salt.yav.get('ver-01dvwxq4yewr4h783qmvehtwn2[password]') }}
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
                password: {{ salt.yav.get('ver-01dvwxshjmnnwn1etha92aab0n[password]') }}
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
                password: {{ salt.yav.get('ver-01dvwxvbcmz7zg6yex26g3aahv[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - '*'

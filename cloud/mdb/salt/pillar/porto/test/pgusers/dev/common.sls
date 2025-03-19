data:
    config:
        pgusers:
            monitor:
                password: {{ salt.yav.get('ver-01e984pkfjm5zvfaxcn7y0rpes[password]') }}
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
                password: {{ salt.yav.get('ver-01e984r5430rga52xksa8zzstr[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: True
                create: True
                bouncer: False
            admin:
                password: {{ salt.yav.get('ver-01e984vrdaj4yxd9qt7y3rw1et[password]') }}
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
                password: {{ salt.yav.get('ver-01e984xb1fmdr9pxk7zh1hvye9[password]') }}
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
                password: {{ salt.yav.get('ver-01e9856r8r97hyy4p4ta86cvc3[password]') }}
                allow_port: '*'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - '*'

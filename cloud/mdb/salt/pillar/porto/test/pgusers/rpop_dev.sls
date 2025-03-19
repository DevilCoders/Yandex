data:
    config:
        pgusers:
            rpop_dev:
                password: {{ salt.yav.get('ver-01dw72x7try858hsb5812tkxmw[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: True
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - rpopdb
                    - rpopdb_grascm
                    - rpopdb_egorp
                settings:
                    log_statement: 'mod'

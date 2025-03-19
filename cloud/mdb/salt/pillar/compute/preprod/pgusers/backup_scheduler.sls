data:
    config:
        pgusers:
            backup_scheduler:
                password: {{ salt.yav.get('ver-01eryd591m6az9r38t0e9gr0g8[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'dbaas_metadb'

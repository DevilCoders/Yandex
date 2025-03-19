data:
    config:
        pgusers:
            backup_scheduler:
                password: {{ salt.yav.get('ver-01erycznawt0h65khmcrbw4faz[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'dbaas_metadb'

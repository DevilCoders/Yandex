data:
    config:
        pgusers:
            backup_cli:
                password: {{ salt.yav.get('ver-01ezqdpbwvvmkfxcyvkdhdgexp[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'dbaas_metadb'

data:
    config:
        pgusers:
            backup_worker:
                password: {{ salt.yav.get('ver-01eryd1hakeg5pjmee6bkanx8y[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'dbaas_metadb'

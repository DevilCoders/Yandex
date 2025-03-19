data:
    config:
        pgusers:
            backup_worker:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 32
                connect_dbs:
                - dbaas_metadb
                create: true
                replication: false
                superuser: false

data:
    config:
        pgusers:
            metadb_admin:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 100
                connect_dbs:
                - dbaas_metadb
                create: true
                replication: false
                superuser: false

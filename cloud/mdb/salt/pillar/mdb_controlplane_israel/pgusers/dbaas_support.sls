data:
    config:
        pgusers:
            dbaas_support:
                allow_db: dbaas_metadb
                allow_port: '6432'
                bouncer: true
                conn_limit: 10
                connect_dbs:
                - dbaas_metadb
                create: true
                replication: false
                superuser: false

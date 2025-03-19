data:
    config:
        pgusers:
            mdb_search_producer:
                allow_db: dbaas_metadb
                allow_port: '6432'
                bouncer: true
                conn_limit: 32
                connect_dbs:
                - dbaas_metadb
                create: true
                replication: false
                superuser: false

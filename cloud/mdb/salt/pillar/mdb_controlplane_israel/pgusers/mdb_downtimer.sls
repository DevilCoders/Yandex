data:
    config:
        pgusers:
            mdb_downtimer:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 5
                connect_dbs:
                - dbaas_metadb
                create: true
                replication: false
                superuser: false

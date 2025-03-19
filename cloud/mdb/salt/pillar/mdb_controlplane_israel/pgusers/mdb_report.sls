data:
    config:
        pgusers:
            mdb_report:
                allow_db: dbaas_metadb
                allow_port: '6432'
                bouncer: true
                connect_dbs:
                - dbaas_metadb
                create: true
                replication: false
                superuser: false

data:
    config:
        pgusers:
            cloud_dwh:
                allow_db: dbaas_metadb
                allow_port: '6432'
                bouncer: true
                conn_limit: 10
                connect_dbs:
                - dbaas_metadb
                create: true
                replication: true
                superuser: false

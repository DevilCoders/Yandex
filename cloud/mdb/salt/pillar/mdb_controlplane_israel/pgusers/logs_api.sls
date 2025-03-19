data:
    config:
        pgusers:
            logs_api:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 5
                connect_dbs:
                - dbaas_metadb
                create: true
                login: false
                replication: false
                superuser: false

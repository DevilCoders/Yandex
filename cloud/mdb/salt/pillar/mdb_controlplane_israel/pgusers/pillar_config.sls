data:
    config:
        pgusers:
            pillar_config:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 100
                connect_dbs:
                - dbaas_metadb
                create: true
                login: false
                replication: false
                superuser: false

data:
    config:
        pgusers:
            cms:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 100
                connect_dbs:
                - cmsdb
                - dbaas_metadb
                create: true
                replication: false
                superuser: false

data:
    config:
        pgusers:
            mdb_ui:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 32
                connect_dbs:
                - deploydb
                - dbaas_metadb
                - katandb
                - cmsdb
                - mlockdb
                create: true
                replication: false
                superuser: false

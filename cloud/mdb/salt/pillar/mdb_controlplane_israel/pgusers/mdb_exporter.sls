data:
    config:
        pgusers:
            mdb_exporter:
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 5
                connect_dbs:
                    - 'dbaas_metadb'

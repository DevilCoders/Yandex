data:
    config:
        pgusers:
            mdb_downtimer:
                password: {{ salt.yav.get('ver-01f8px7q7chmtz7wkm6rd6khmb[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 5
                connect_dbs:
                    - dbaas_metadb

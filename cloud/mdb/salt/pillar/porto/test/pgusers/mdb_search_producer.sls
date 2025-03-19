data:
    config:
        pgusers:
            mdb_search_producer:
                password: {{ salt.yav.get('ver-01dvxcq5r8waydckmjy2w3k90q[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - dbaas_metadb

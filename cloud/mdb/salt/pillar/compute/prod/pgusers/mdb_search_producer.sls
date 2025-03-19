data:
    config:
        pgusers:
            mdb_search_producer:
                password: {{ salt.yav.get('ver-01e98cmvv4wg40snskr6rbm01v[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - dbaas_metadb

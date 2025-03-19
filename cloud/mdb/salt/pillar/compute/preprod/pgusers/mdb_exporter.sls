data:
    config:
        pgusers:
            mdb_exporter:
                password: {{ salt.yav.get('ver-01g7pz9r8jgyekjzp5ajfncvwt[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 5
                connect_dbs:
                    - 'dbaas_metadb'

data:
    config:
        pgusers:
            mdb_event_producer:
                password: {{ salt.yav.get('ver-01dzttc058nj9s3kxxet2t89pq[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - dbaas_metadb

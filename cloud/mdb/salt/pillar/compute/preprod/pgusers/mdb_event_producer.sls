data:
    config:
        pgusers:
            mdb_event_producer:
                password: {{ salt.yav.get('ver-01dws6j7d3hnhj4svgya4twrcq[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - dbaas_metadb

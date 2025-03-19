data:
    config:
        pgusers:
            mdb_health:
                password: {{ salt.yav.get('ver-01e39r3wdwc5c5bqdva1r1c4q0[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

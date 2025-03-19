data:
    config:
        pgusers:
            mdb_report:
                password: {{ salt.yav.get('ver-01dws68pz7xksephmsz719a6ac[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

data:
    config:
        pgusers:
            mdb_health:
                password: {{ salt.yav.get('ver-01dznyrcya1qp4rynb475kjxbh[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

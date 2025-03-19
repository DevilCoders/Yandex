data:
    config:
        pgusers:
            mdb_maintenance:
                password: {{ salt.yav.get('ver-01e70b4avzh75dgax11p726fmn[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

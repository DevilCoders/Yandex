data:
    config:
        pgusers:
            mdb_maintenance:
                password: {{ salt.yav.get('ver-01e70csxrgtgbsxtee3hy2yeag[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

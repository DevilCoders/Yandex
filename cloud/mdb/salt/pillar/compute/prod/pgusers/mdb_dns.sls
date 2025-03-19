data:
    config:
        pgusers:
            mdb_dns:
                password: {{ salt.yav.get('ver-01e2g8rtedva84vq061jyedr6a[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

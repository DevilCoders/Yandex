data:
    config:
        pgusers:
            dbaas_worker:
                password: {{ salt.yav.get('ver-01e8v0kdjgfejr9t4xdf3dz2z2[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

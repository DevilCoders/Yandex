data:
    config:
        pgusers:
            mdb_maintenance:
                password: {{ salt.yav.get('ver-01e70cpf4hsm9qe6n4xwt8frq6[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

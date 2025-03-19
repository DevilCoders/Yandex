data:
    config:
        pgusers:
            dbaas_api:
                password: {{ salt.yav.get('ver-01dzb4nyjp1jkh8gxaftptktjg[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

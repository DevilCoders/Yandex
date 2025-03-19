data:
    config:
        pgusers:
            dbaas_api:
                password: {{ salt.yav.get('ver-01e87pdajgwnnenhj9qy64e618[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

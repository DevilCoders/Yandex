data:
    config:
        pgusers:
            idm_service:
                password: {{ salt.yav.get('ver-01e999897xscetxxb3vm9skg4a[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

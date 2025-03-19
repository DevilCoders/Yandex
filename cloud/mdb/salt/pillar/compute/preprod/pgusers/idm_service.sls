data:
    config:
        pgusers:
            idm_service:
                password: {{ salt.yav.get('ver-01dws6b4w98cvprr7px8h2r2rm[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

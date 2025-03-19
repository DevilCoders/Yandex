data:
    config:
        pgusers:
            idm_service:
                password: {{ salt.yav.get('ver-01dztt25bgy4a9tfenkyn6zs1e[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

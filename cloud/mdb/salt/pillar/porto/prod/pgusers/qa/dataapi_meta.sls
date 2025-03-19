data:
    config:
        pgusers:
            dataapi_meta:
                password: {{ salt.yav.get('ver-01e205sm8hefacw9bgjaeb6spg[password]') }}
                allow_port: '6432'
                allow_db: dataapi_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dataapi_metadb

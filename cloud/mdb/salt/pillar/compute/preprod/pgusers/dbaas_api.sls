data:
    config:
        pgusers:
            dbaas_api:
                password: {{ salt.yav.get('ver-01dwew18xwjg8mq1jy8jrea2qg[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

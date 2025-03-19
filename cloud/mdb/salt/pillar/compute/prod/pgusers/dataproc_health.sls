data:
    config:
        pgusers:
            dataproc_health:
                password: {{ salt.yav.get('ver-01eh7d97qnd0v7e26q05jb2ske[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

data:
    config:
        pgusers:
            cms:
                password: {{ salt.yav.get('ver-01e43k7em77zrmx608bmzjr1xz[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - cmsdb
                    - dbaas_metadb

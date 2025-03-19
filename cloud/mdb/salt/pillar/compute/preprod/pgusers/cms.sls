data:
    config:
        pgusers:
            cms:
                password: {{ salt.yav.get('ver-01eybc6y3z0m5rm999b2zyc1wh[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - cmsdb
                    - dbaas_metadb

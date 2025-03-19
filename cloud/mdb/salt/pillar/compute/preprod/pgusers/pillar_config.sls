data:
    config:
        pgusers:
            pillar_config:
                password: {{ salt.yav.get('ver-01fqkkn0qrfg5j0k1qyapceh5x[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                login: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - dbaas_metadb

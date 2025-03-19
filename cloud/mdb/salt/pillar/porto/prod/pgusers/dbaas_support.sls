data:
    config:
        pgusers:
            dbaas_support:
                password: {{ salt.yav.get('ver-01dztt4yrnvvjz9sz1f3b5x3f9[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 10
                connect_dbs:
                    - 'dbaas_metadb'

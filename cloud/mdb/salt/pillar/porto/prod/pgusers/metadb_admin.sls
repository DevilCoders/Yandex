data:
    config:
        pgusers:
            metadb_admin:
                password: {{ salt.yav.get('ver-01fpyhve8cx9zw05p9rbgmbj96[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - dbaas_metadb

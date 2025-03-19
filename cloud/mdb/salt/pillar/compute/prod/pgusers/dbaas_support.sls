data:
    config:
        pgusers:
            dbaas_support:
                password: {{ salt.yav.get('ver-01e9995a8d0th84ps5r8enq45e[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 10
                connect_dbs:
                    - 'dbaas_metadb'

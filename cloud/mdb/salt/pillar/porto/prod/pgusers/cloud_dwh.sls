data:
    config:
        pgusers:
            cloud_dwh:
                password: {{ salt.yav.get('ver-01fxqqpjwf25103asb67s0kp79[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: True
                create: True
                bouncer: True
                conn_limit: 10
                connect_dbs:
                    - dbaas_metadb

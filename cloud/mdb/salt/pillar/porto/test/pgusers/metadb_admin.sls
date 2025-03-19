data:
    config:
        pgusers:
            metadb_admin:
                password: {{ salt.yav.get('ver-01fpyhjrxattxjtdznywt2pdbe[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - dbaas_metadb

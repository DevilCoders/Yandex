data:
    config:
        pgusers:
            mlockdb_admin:
                password: {{ salt.yav.get('ver-01fr09hh07fnyj6x7hachdee6d[password]') }}
                allow_port: '6432'
                allow_db: 'mlockdb'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - 'mlockdb'

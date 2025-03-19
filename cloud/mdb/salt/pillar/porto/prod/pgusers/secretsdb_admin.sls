data:
    config:
        pgusers:
            secretsdb_admin:
                password: {{ salt.yav.get('ver-01fq1pde7af7e0bxhhxq4ndd81[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - secretsdb

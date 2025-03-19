data:
    config:
        pgusers:
            secretsdb_admin:
                password: {{ salt.yav.get('ver-01fq1pj5jzgb84q86pj2rrh0zf[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - secretsdb

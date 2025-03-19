data:
    config:
        pgusers:
            s3ro:
                password: {{ salt.yav.get('ver-01e1c5h0p33ra1czav0vq1nm9j[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 300
                connect_dbs:
                    - s3db 
                pgpass: True
                port: 6432

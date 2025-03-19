data:
    config:
        pgusers:
            cmsdb_admin:
                password: {{ salt.yav.get('ver-01fr51ea85eg1d4923n4yx5k2f[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - cmsdb

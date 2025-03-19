data:
    config:
        pgusers:
            cmsdb_admin:
                password: {{ salt.yav.get('ver-01fr5205s159m7wnch8e5k05pe[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - cmsdb

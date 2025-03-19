data:
    config:
        pgusers:
            deploy_api:
                password: {{ salt.yav.get('ver-01dwh2azbpm53bae023jeygedy[password]') }}
                allow_port: '6432'
                allow_db: deploydb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - deploydb

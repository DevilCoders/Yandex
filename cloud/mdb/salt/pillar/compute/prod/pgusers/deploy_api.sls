data:
    config:
        pgusers:
            deploy_api:
                password: {{ salt.yav.get('ver-01e48pznw2zxv12hyk5c7pge83[password]') }}
                allow_port: '6432'
                allow_db: deploydb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - deploydb

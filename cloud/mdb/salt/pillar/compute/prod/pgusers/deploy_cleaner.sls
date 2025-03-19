data:
    config:
        pgusers:
            deploy_cleaner:
                password: {{ salt.yav.get('ver-01e48q1ayk2j7ahr2wh0gyg06t[password]') }}
                allow_port: '6432'
                allow_db: deploydb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - deploydb

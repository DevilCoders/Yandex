data:
    config:
        pgusers:
            deploy_api:
                password: {{ salt.yav.get('ver-01dvwm7syf70bqt1a4z6k6ekev[password]') }}
                allow_port: '6432'
                allow_db: deploydb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - deploydb

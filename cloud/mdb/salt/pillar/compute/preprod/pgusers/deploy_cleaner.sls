data:
    config:
        pgusers:
            deploy_cleaner:
                password: {{ salt.yav.get('ver-01e3q267zpx60qw2qpd8v3rcm0[password]') }}
                allow_port: '6432'
                allow_db: deploydb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - deploydb

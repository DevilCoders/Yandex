data:
    config:
        pgusers:
            deploy_cleaner:
                allow_db: deploydb
                allow_port: '6432'
                bouncer: true
                connect_dbs:
                - deploydb
                create: true
                replication: false
                superuser: false

data:
    config:
        pgusers:
            s3ro:
                allow_db: '*'
                allow_port: '6432'
                bouncer: true
                conn_limit: 300
                connect_dbs:
                - s3db
                create: true
                password: '{{ salt.lockbox.get("bcnfeac3ogph7gf1003k").password }}'
                pgpass: true
                port: 6432
                replication: false
                superuser: false

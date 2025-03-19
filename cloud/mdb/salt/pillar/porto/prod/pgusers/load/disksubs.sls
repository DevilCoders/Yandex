data:
    config:
        pgusers:
            disksubs:
                password: {{ salt.yav.get('ver-01e11hk7t9rrjyprn6gtrrfmvg[password]') }}
                allow_port: '6432'
                allow_db: disksubsdb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - disksubsdb

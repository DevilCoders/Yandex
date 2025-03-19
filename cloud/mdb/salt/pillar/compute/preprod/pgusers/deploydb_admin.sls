data:
    config:
        pgusers:
            deploydb_admin:
                password: {{ salt.yav.get('ver-01fpstv6je7ahskb0q10pgz59q[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 100
                connect_dbs:
                    - deploydb

data:
    config:
        pgusers:
            rpop:
                password: {{ salt.yav.get('ver-01e23b6eg37bwpnwt81aystkpy[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - rpopdb
            rpoptmp:
                password: {{ salt.yav.get('ver-01e23b8f0x1jcdbcnjyt78gx3m[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - rpopdb

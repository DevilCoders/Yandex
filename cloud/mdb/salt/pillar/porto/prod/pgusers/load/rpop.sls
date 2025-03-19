data:
    config:
        pgusers:
            rpop:
                password: {{ salt.yav.get('ver-01e19fz5xhtywmacnxshqt3r6z[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - rpopdb
            rpoptmp:
                password: {{ salt.yav.get('ver-01e11n25pz419x14kszw1m6sgh[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - rpopdb

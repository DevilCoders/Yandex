data:
    config:
        pgusers:
            rpop_transfer:
                password: {{ salt.yav.get('ver-01e23b9rxakbzdvhas37ghkw7n[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 50
                connect_dbs:
                    - '*'
            rpop_transfer_tmp:
                password: {{ salt.yav.get('ver-01e23bak29b0t1bcj4rawnrr58[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 50
                connect_dbs:
                    - '*'

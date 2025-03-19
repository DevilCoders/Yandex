data:
    config:
        pgusers:
            rpop:
                password: {{ salt.yav.get('ver-01dw72sm2twx366sb33zqg1xb4[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - rpopdb
                    - rpopdb_grascm
                    - rpopdb_egorp
            rpoptmp:
                password: {{ salt.yav.get('ver-01dw72tmjcjn1f2cv9jg4rjq30[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - rpopdb
                    - rpopdb_grascm
                    - rpopdb_egorp

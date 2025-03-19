data:
    config:
        pgusers:
            rpop:
                password: {{ salt.yav.get('ver-01e2058c7sbncpsvkjgk7d5hwf[password]') }} 
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - rpopdb
            rpoptmp:
                password: {{ salt.yav.get('ver-01e20594svp9x9bnj0pc0as65p[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - rpopdb

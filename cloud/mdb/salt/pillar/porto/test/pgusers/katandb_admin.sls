data:
    config:
        pgusers:
            katandb_admin:
                password: {{ salt.yav.get('ver-01fr5nzfbgem9k4366hcynk2e7[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 64
                connect_dbs:
                    - 'katandb'

data:
    config:
        pgusers:
            katandb_admin:
                password: {{ salt.yav.get('ver-01fr5p2wnx4wf267t7xw6x3mra[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 64
                connect_dbs:
                    - 'katandb'

data:
    config:
        pgusers:
            backup_scheduler:
                password: {{ salt.yav.get('ver-01erydjapb4zp0850qtdxxks16[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'dbaas_metadb'

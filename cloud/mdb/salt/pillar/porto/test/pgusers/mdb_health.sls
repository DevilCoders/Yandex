data:
    config:
        pgusers:
            mdb_health:
                password: {{ salt.yav.get('ver-01dvxc1g5w19qsr7z0psfjbm49[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

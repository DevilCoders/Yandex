data:
    config:
        pgusers:
            mdb_health:
                password: {{ salt.yav.get('ver-01dws5kr0djdeeg5zjz20e70xd[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

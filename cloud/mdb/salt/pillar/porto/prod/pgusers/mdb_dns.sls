data:
    config:
        pgusers:
            mdb_dns:
                password: {{ salt.yav.get('ver-01dznxsm22fzpwmpscraew4e80[password]') }}
                allow_port: '6432'
                allow_db: dbaas_metadb
                superuser: False
                replication: False
                create: True
                bouncer: True
                connect_dbs:
                    - dbaas_metadb

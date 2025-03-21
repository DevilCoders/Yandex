data:
    config:
        pgusers:
            mdb_ui:
                password: {{ salt.yav.get('ver-01eg075spjrkvtzng7zzhhf3dm[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'deploydb'
                    - 'dbaas_metadb'
                    - 'katandb'
                    - 'dbm'
                    - 'cmsdb'
                    - 'mlockdb'

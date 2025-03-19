data:
    config:
        pgusers:
            mdb_ui:
                password: {{ salt.yav.get('ver-01eg07gmcbtcg4bs75a2q51sh3[password]') }}
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
                    - 'cmsdb'
                    - 'mlockdb'

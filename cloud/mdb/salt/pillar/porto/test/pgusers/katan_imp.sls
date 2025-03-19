data:
    config:
        pgusers:
            katan_imp:
                password: {{ salt.yav.get('ver-01e0znvghkwr6rc6qmpqyzmree[password]') }}
                allow_port: '6432'
                allow_db: '*'
                superuser: False
                replication: False
                create: True
                bouncer: True
                conn_limit: 32
                connect_dbs:
                    - 'katandb'
                    - 'dbaas_metadb'
